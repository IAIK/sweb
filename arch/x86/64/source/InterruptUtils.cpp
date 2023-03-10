#include "InterruptUtils.h"

#include "ArchSerialInfo.h"
#include "BDManager.h"
#include "new.h"
#include "ports.h"
#include "ArchMemory.h"
#include "ArchThreads.h"
#include "ArchCommon.h"
#include "Console.h"
#include "Terminal.h"
#include "kprintf.h"
#include "Scheduler.h"
#include "debug_bochs.h"
#include "offsets.h"
#include "kstring.h"

#include "SerialManager.h"
#include "KeyboardManager.h"
#include "panic.h"

#include "Thread.h"
#include "ArchInterrupts.h"
#include "ArchMulticore.h"
#include "backtrace.h"

#include "SWEBDebugInfo.h"
#include "Loader.h"
#include "Syscall.h"
#include "paging-definitions.h"
#include "PageFaultHandler.h"
#include "TimerTickHandler.h"
#include "ErrorHandlers.h"

#include "8259.h"

extern "C" void arch_dummyHandler();
extern "C" void arch_dummyHandlerMiddle();

InterruptDescriptorTable InterruptUtils::idt;

extern const SWEBDebugInfo* kernel_debug_info;
extern eastl::atomic_flag assert_print_lock;


// generic interrupt handler -> dispatches to registered handlers for invoked interrupt number
void interruptHandler(size_t interrupt_num,
                      uint64_t error_code,
                      ArchThreadRegisters* saved_registers)
{
    debugAdvanced(A_INTERRUPTS, "[CPU %zu] Interrupt handler %zu, error: %lx\n",
        SMP::currentCpuId(), interrupt_num, error_code);
    assert(interrupt_num < 256);

    if (interrupt_num == 0xE)
    {
        uintptr_t pagefault_addr = 0;
        asm volatile("movq %%cr2, %[cr2]\n" : [cr2] "=r"(pagefault_addr));
        pageFaultHandler(pagefault_addr, error_code, saved_registers->rip);
    }
    else if (interrupt_num < 32)
    {
        errorHandler(interrupt_num, saved_registers->rip, saved_registers->cs, 0);
    }
    else
    {
        ArchInterrupts::startOfInterrupt(interrupt_num);
        ArchInterrupts::handleInterrupt(interrupt_num);
        ArchInterrupts::endOfInterrupt(interrupt_num);
    }

    debugAdvanced(A_INTERRUPTS, "Interrupt handler %zu end\n", interrupt_num);
}

/**
 * Interrupt handler for Programmable Interval Timer (PIT)
 * ISA IRQ 0 -> remapped to interrupt vector 32 via PIC 8259/IoApic
 */
void int32_handler_PIT_irq0()
{
    debugAdvanced(A_INTERRUPTS, "[CPU %zu] Interrupt vector %u (ISA IRQ %u) called\n",
        SMP::currentCpuId(), InterruptVector::REMAP_OFFSET + (uint8_t)ISA_IRQ::PIT, 0);

    ArchCommon::callWithStack(
        ArchMulticore::cpuStackTop(),
        []()
        {
            TimerTickHandler::handleTimerTick();

            ((char*)ArchCommon::getFBPtr())[1 + SMP::currentCpuId() * 2] =
                ((currentThread->console_color << 4) | CONSOLECOLOR::BRIGHT_WHITE);

            // Signal end of interrupt here since we don't return normally
            ArchInterrupts::endOfInterrupt(InterruptVector::REMAP_OFFSET + (uint8_t)ISA_IRQ::PIT);
            contextSwitch();
            assert(false);
        });
}

/**
 * Interrupt handler for APIC timer
 */
void int127_handler_APIC_timer()
{
    debugAdvanced(A_INTERRUPTS, "[CPU %zu] Interrupt vector %u called\n",
        SMP::currentCpuId(), InterruptVector::APIC_TIMER);

    ArchCommon::callWithStack(ArchMulticore::cpuStackTop(),
        []()
        {
            TimerTickHandler::handleTimerTick();

            ((char*)ArchCommon::getFBPtr())[1 + SMP::currentCpuId()*2] =
                ((currentThread->console_color << 4) |
                 CONSOLECOLOR::BRIGHT_WHITE);

            // Signal end of interrupt here since we don't return normally
            ArchInterrupts::endOfInterrupt(InterruptVector::APIC_TIMER);
            contextSwitch();
            assert(false);
        });
}


/**
 * Interrupt handler for software interrupt 65 used by yield()
 */
void int65_handler_swi_yield()
{
    debugAdvanced(A_INTERRUPTS, "[CPU %zu] Interrupt vector %u called\n",
        SMP::currentCpuId(), InterruptVector::YIELD);

    ArchCommon::callWithStack(
        ArchMulticore::cpuStackTop(),
        []()
        {
            Scheduler::instance()->schedule();

            ((char*)ArchCommon::getFBPtr())[1 + SMP::currentCpuId() * 2] =
                ((currentThread->console_color << 4) | CONSOLECOLOR::BRIGHT_WHITE);

            // Signal end of interrupt here since we don't return normally
            ArchInterrupts::endOfInterrupt(InterruptVector::YIELD);
            contextSwitch();
            assert(false);
        });
}

/**
 * Handler for pagefault exception
 *
 * @param address accessed address that caused the pagefault
 * @param error information about the pagefault provided by the CPU
 * @param ip CPU instruction pointer at the pagefault
 */
void pageFaultHandler(uint64_t address, uint64_t error, uint64_t ip)
{
    auto &regs = *(currentThread->switch_to_userspace_ ? currentThread->user_registers_ :
                                                         currentThread->kernel_registers_);

    // dirty hack due to qemu invoking the pf handler when accessing non canonical addresses
    if (address >= USER_BREAK && address < KERNEL_START)
    {
        errorHandler(0xd, regs.rip, regs.cs, 0);
        assert(false && "thread should not survive a GP fault");
    }
    PagefaultExceptionErrorCode error_code{static_cast<uint32_t>(error)};
    assert(!error_code.reserved_write && "Reserved bit set in page table entry");

    assert(ArchThreads::getInterruptEnableFlag(regs) && "PF with interrupts disabled. PF handler will enable interrupts soon. Better panic now");

    PageFaultHandler::enterPageFault(address, ip, error_code.user, error_code.present,
                                     error_code.write, error_code.instruction_fetch);
    if (currentThread->switch_to_userspace_)
        contextSwitch();
    else
        asm volatile ("movq %%cr3, %%rax; movq %%rax, %%cr3;" ::: "%rax");
}

/**
 * Handler for Inter-Processor-Interrupt vector 90.
 * Used to stop other CPUs when a kernel panic occurs.
 */
void int90_handler_halt_cpu()
{
    while (assert_print_lock.test_and_set(eastl::memory_order_acquire));

    debugAlways(A_INTERRUPTS, "Interrupt %u called, CPU %zu halting\n", InterruptVector::IPI_HALT_CPU, SMP::currentCpuId());
    if (currentThread)
    {
        debug(BACKTRACE, "CPU %zu backtrace:\n", SMP::currentCpuId());
        currentThread->printBacktrace(false);
    }

    assert_print_lock.clear(eastl::memory_order_release);

    while(true)
    {
        ArchCommon::halt();
    }
}

/**
 * APIC error interrupt handler.
 * Invoked by the APIC when it encounters an error condition.
 */
void int91_handler_APIC_error()
{
    auto error = cpu_lapic->readRegister<Apic::Register::ERROR_STATUS>();
    debugAlways(APIC, "Internal APIC error: %x\n", *(uint32_t*)&error);

    assert(!"Internal APIC error");

    cpu_lapic->writeRegister<Apic::Register::ERROR_STATUS>({});
}

/**
 * Handler for spurious APIC interrupts.
 * Spurious interrupts can be triggered under special conditions when an interrupt signal
 * becomes masked at the same time it is raised. Can safely be ignored.
 */
void int100_handler_APIC_spurious()
{
    debug(A_INTERRUPTS, "IRQ %u called by CPU %zu, spurious APIC interrupt\n", InterruptVector::APIC_SPURIOUS, SMP::currentCpuId());
}

/**
 * Handler for Inter-Processor-Interrupt vector 100.
 * Used to send remote function call requests to other CPUs (e.g. TLB flush requests)
 */
void int101_handler_cpu_fcall()
{
    debug(A_INTERRUPTS, "IRQ %u called by CPU %zu\n", InterruptVector::IPI_REMOTE_FCALL, SMP::currentCpuId());

    auto funcdata = SMP::currentCpu().fcall_queue.takeAll();
    while (funcdata != nullptr)
    {
        debug(A_INTERRUPTS, "CPU %zu: Function call request from CPU %zu\n", SMP::currentCpuId(), funcdata->orig_cpu);

        funcdata->received.store(true, eastl::memory_order_release);

        assert(funcdata->target_cpu == SMP::currentCpuId());
        assert(funcdata->func);

        funcdata->func();

        auto next = funcdata->next.load();
        funcdata->done.store(true, eastl::memory_order_release); // funcdata object is invalid as soon as it is acknowledged
        funcdata = next;
    }
}

/**
 * Handler for syscall software interrupts from userspace.
 * Marks thread as being in kernel mode, reenables interrupts, and passes syscall
 * parameters stored in user registers to the generic syscall handler.
 */
void syscallHandler()
{
    currentThread->switch_to_userspace_ = 0;
    currentThreadRegisters = currentThread->kernel_registers_.get();
    ArchInterrupts::enableInterrupts();

    auto ret = Syscall::syscallException(currentThread->user_registers_->rax,
                                        currentThread->user_registers_->rbx,
                                        currentThread->user_registers_->rcx,
                                        currentThread->user_registers_->rdx,
                                        currentThread->user_registers_->rsi,
                                        currentThread->user_registers_->rdi);

    currentThread->user_registers_->rax = ret;

    ArchInterrupts::disableInterrupts();
    currentThread->switch_to_userspace_ = 1;
    currentThreadRegisters = currentThread->user_registers_.get();
    contextSwitch();
    assert(false);
}

/**
 * Handler for exception interrupts raised by the CPU (e.g. invalid opcode, divide by zero, general protection fault, ...)
 *
 * @param num raised exception number
 * @param rip instruction pointer when the exception was raised
 * @param cs selected code segment when the exception was raised
 * @param spurious whether the exception was a spurious interrupt rather than a CPU exception
 */
void errorHandler(size_t num, size_t rip, size_t cs, size_t spurious)
{
  kprintfd("%zx\n",cs);
  if (spurious)
  {
    assert(num < 256 && "there are only 256 interrupts");
    debug(CPU_ERROR, "Spurious Interrupt %zu (%zx)\n", num, num);
  }
  else
  {
    assert(num < 32 && "there are only 32 CPU errors");
    debug(CPU_ERROR, "\033[1;31m%s\033[0;39m\n", errors[num]);
  }
  const bool userspace = (cs & 0x3);
  debug(CPU_ERROR, "Instruction Pointer: %zx, Userspace: %d - currentThread(): %p %zd" ":%s, switch_to_userspace_: %d\n",
        rip, userspace, currentThread,
        currentThread ? currentThread->getTID() : -1UL, currentThread ? currentThread->getName() : 0,
        currentThread ? currentThread->switch_to_userspace_ : -1);

  const Stabs2DebugInfo* deb = kernel_debug_info;
  assert(currentThread && "there should be no fault before there is a current thread");
  assert(currentThread->kernel_registers_ && "every thread needs kernel registers");
  if (userspace)
  {
    assert(currentThread->loader_ && "User Threads need to have a Loader");
    assert(currentThread->user_registers_ && (currentThread->user_registers_->cr3 == currentThread->kernel_registers_->cr3 &&
           "User and Kernel CR3 register values differ, this most likely is a bug!"));
    deb = currentThread->loader_->getDebugInfos();
  }
  if(deb && rip)
  {
    debug(CPU_ERROR, "This Fault was probably caused by:");
    deb->printCallInformation(rip);
  }
  ArchThreads::printThreadRegisters(currentThread, false);
  currentThread->printBacktrace(true);

  if (spurious)
  {
    if (currentThread->switch_to_userspace_)
    {
      contextSwitch();
      assert(false);
    }
  }
  else
  {
    currentThread->switch_to_userspace_ = false;
    currentThreadRegisters = currentThread->kernel_registers_.get();
    ArchInterrupts::enableInterrupts();
    debug(CPU_ERROR, "Terminating process...\n");
    if (currentThread->user_registers_)
    {
      Syscall::exit(888);
    }
    else
    {
      currentThread->kill();
    }

    assert(false);
  }
}
