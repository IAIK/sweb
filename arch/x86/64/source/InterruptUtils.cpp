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

// Standard ISA IRQs
enum ISA_IRQ
{
    PIT              = 0, // 0 	Programmable Interrupt Timer Interrupt
    KEYBOARD         = 1, // 1 	Keyboard Interrupt
    PIC_8259_CASCADE = 2, // 2 	Cascade (used internally by the two PICs. never raised)
    COM2             = 3, // 3 	COM2 (if enabled)
    COM1             = 4, // 4 	COM1 (if enabled)
    LPT2             = 5, // 5 	LPT2 (if enabled)
    FLOPPY_DISK      = 6, // 6 	Floppy Disk
    LPT1             = 7, // 7 	LPT1 / Unreliable "spurious" interrupt (usually)
    RTC              = 8, // 8 	CMOS real-time clock (if enabled)
    // 9 	Free for peripherals / legacy SCSI / NIC
    // 10 	Free for peripherals / SCSI / NIC
    // 11 	Free for peripherals / SCSI / NIC
    PS2_MOUSE        = 12, // 12 	PS2 Mouse
    FPU              = 13, // 13 	FPU / Coprocessor / Inter-processor
    ATA_PRIMARY      = 14, // 14 	Primary ATA Hard Disk
    ATA_SECONDARY    = 15, // 15 	Secondary ATA Hard Disk
};

void irqHandler_0()
{
    debugAdvanced(A_INTERRUPTS, "[CPU %zu] Interrupt vector %u (ISA IRQ %u) called\n",
          SMP::currentCpuId(), Apic::IRQ_VECTOR_OFFSET + 0, 0);

    ArchCommon::callWithStack(
        ArchMulticore::cpuStackTop(),
        []()
        {
            TimerTickHandler::handleTimerTick();

            ((char*)ArchCommon::getFBPtr())[1 + SMP::currentCpuId() * 2] =
                ((currentThread->console_color << 4) | CONSOLECOLOR::BRIGHT_WHITE);

            // Signal end of interrupt here since we don't return normally
            ArchInterrupts::endOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 0);
            contextSwitch();
            assert(false);
        });
}

void irqHandler_127()
{
    debugAdvanced(A_INTERRUPTS, "[CPU %zu] Interrupt vector %u called\n",
                  SMP::currentCpuId(), 127);

    ArchCommon::callWithStack(ArchMulticore::cpuStackTop(), []()
        {
            TimerTickHandler::handleTimerTick();

            ((char*)ArchCommon::getFBPtr())[1 + SMP::currentCpuId()*2] =
                ((currentThread->console_color << 4) |
                 CONSOLECOLOR::BRIGHT_WHITE);

            // Signal end of interrupt here since we don't return normally
            ArchInterrupts::endOfInterrupt(127);
            contextSwitch();
            assert(false);
        });
}


// yield
void irqHandler_65()
{
    debugAdvanced(A_INTERRUPTS, "[CPU %zu] Interrupt vector %u called\n",
                  SMP::currentCpuId(), 65);

    ArchCommon::callWithStack(
        ArchMulticore::cpuStackTop(),
        []()
        {
            Scheduler::instance()->schedule();

            ((char*)ArchCommon::getFBPtr())[1 + SMP::currentCpuId() * 2] =
                ((currentThread->console_color << 4) | CONSOLECOLOR::BRIGHT_WHITE);

            // Signal end of interrupt here since we don't return normally
            ArchInterrupts::endOfInterrupt(65);
            contextSwitch();
            assert(false);
        });
}

void pageFaultHandler(uint64_t address, uint64_t error, uint64_t ip)
{
  if (address >= USER_BREAK && address < KERNEL_START) { // dirty hack due to qemu invoking the pf handler when accessing non canonical addresses
      auto &regs = *(currentThread->switch_to_userspace_ ? currentThread->user_registers_ : currentThread->kernel_registers_);
      errorHandler(0xd, regs.rip, regs.cs, 0);
      assert(0 && "thread should not survive a GP fault");
  }
  PagefaultExceptionErrorCode error_code{static_cast<uint32_t>(error)};
  assert(!error_code.reserved_write && "Reserved bit set in page table entry");

  PageFaultHandler::enterPageFault(address, ip, error_code.user, error_code.present,
                                   error_code.write, error_code.instruction_fetch);
  if (currentThread->switch_to_userspace_)
    contextSwitch();
  else
    asm volatile ("movq %%cr3, %%rax; movq %%rax, %%cr3;" ::: "%rax");
}

void irqHandler_90()
{
    while (assert_print_lock.test_and_set(eastl::memory_order_acquire));

    debug(A_INTERRUPTS, "IRQ 90 called, CPU %zu halting\n", SMP::currentCpuId());
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

void irqHandler_91()
{
    auto error = cpu_lapic->readRegister<Apic::Register::ERROR_STATUS>();
    debugAlways(APIC, "Internal APIC error: %x\n", *(uint32_t*)&error);

    assert(!"Internal APIC error");

    cpu_lapic->writeRegister<Apic::Register::ERROR_STATUS>({});
}

void irqHandler_100()
{
    debug(A_INTERRUPTS, "IRQ 100 called by CPU %zu, spurious APIC interrupt\n", SMP::currentCpuId());
}

void irqHandler_101()
{
    debug(A_INTERRUPTS, "IRQ 101 called by CPU %zu\n", SMP::currentCpuId());

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
