#include "InterruptUtils.h"

#include "8259.h"
#include "BDManager.h"
#include "Console.h"
#include "ErrorHandlers.h"
#include "KeyboardManager.h"
#include "Loader.h"
#include "PageFaultHandler.h"
#include "Scheduler.h"
#include "SerialManager.h"
#include "Stabs2DebugInfo.h"
#include "Syscall.h"
#include "Thread.h"
#include "TimerTickHandler.h"
#include "backtrace.h"
#include "kprintf.h"
#include "offsets.h"
#include "paging-definitions.h"

#include "ArchCommon.h"
#include "ArchInterrupts.h"
#include "ArchMemory.h"
#include "ArchMulticore.h"
#include "ArchSerialInfo.h"
#include "ArchThreads.h"

// --- Pagefault error flags.
//     PF because/in/caused by/...

#define FLAG_PF_PRESENT     0x01 // =0: pt/page not present
                                 // =1: of protection violation

#define FLAG_PF_RDWR        0x02 // =0: read access
                                 // =1: write access

#define FLAG_PF_USER        0x04 // =0: supervisormode (CPL < 3)
                                 // =1: usermode (CPL == 3)

#define FLAG_PF_RSVD        0x08 // =0: not a reserved bit
                                 // =1: a reserved bit

#define FLAG_PF_INSTR_FETCH 0x10 // =0: not an instruction fetch
                                 // =1: an instruction fetch (need PAE for that)


InterruptDescriptorTable InterruptUtils::idt;

extern const Stabs2DebugInfo* kernel_debug_info;
extern eastl::atomic_flag assert_print_lock;

// Standard ISA IRQs
// 0 	Programmable Interrupt Timer Interrupt
// 1 	Keyboard Interrupt
// 2 	Cascade (used internally by the two PICs. never raised)
// 3 	COM2 (if enabled)
// 4 	COM1 (if enabled)
// 5 	LPT2 (if enabled)
// 6 	Floppy Disk
// 7 	LPT1 / Unreliable "spurious" interrupt (usually)
// 8 	CMOS real-time clock (if enabled)
// 9 	Free for peripherals / legacy SCSI / NIC
// 10 	Free for peripherals / SCSI / NIC
// 11 	Free for peripherals / SCSI / NIC
// 12 	PS2 Mouse
// 13 	FPU / Coprocessor / Inter-processor
// 14 	Primary ATA Hard Disk
// 15 	Secondary ATA Hard Disk

// generic interrupt handler -> dispatches to registered handlers for invoked interrupt number
void interruptHandler(size_t interrupt_num,
                      uint32_t error_code,
                      ArchThreadRegisters* saved_registers)
{
    debugAdvanced(A_INTERRUPTS, "[CPU %zu] Interrupt handler %zu, error: %x\n", SMP::currentCpuId(), interrupt_num, error_code);
    assert(interrupt_num < 256);

    if (interrupt_num == 0xE)
    {
        uintptr_t pagefault_addr = 0;
        asm volatile("movl %%cr2, %[cr2]\n" : [cr2] "=r"(pagefault_addr));
        pageFaultHandler(pagefault_addr, error_code, saved_registers->eip);
    }
    else if (interrupt_num < 32)
    {
        errorHandler(interrupt_num, saved_registers->eip, saved_registers->cs, 0);
    }
    else
    {
        ArchInterrupts::startOfInterrupt(interrupt_num);
        ArchInterrupts::handleInterrupt(interrupt_num);
        ArchInterrupts::endOfInterrupt(interrupt_num);
    }

    debugAdvanced(A_INTERRUPTS, "Interrupt handler %zu end\n", interrupt_num);
}

// ISA IRQ 0 remapped to interrupt vector 32
void int32_handler_PIT_irq0()
{
  debugAdvanced(A_INTERRUPTS, "[CPU %zu] IRQ 0 called\n", SMP::currentCpuId());

  ArchCommon::callWithStack(
      ArchMulticore::cpuStackTop(),
      []()
      {
          TimerTickHandler::handleTimerTick();

          ((char*)ArchCommon::getFBPtr())[1 + SMP::currentCpuId()*2] =
              ((currentThread->console_color << 4) | CONSOLECOLOR::BRIGHT_WHITE);

          ArchInterrupts::endOfInterrupt(InterruptVector::REMAP_OFFSET + (uint8_t)ISA_IRQ::PIT);
          contextSwitch();
          assert(false);
      });
}

void int127_handler_APIC_timer()
{
    debugAdvanced(A_INTERRUPTS, "[CPU %zu] Interrupt vector %u called\n", SMP::currentCpuId(), InterruptVector::APIC_TIMER);

    ArchCommon::callWithStack(
        ArchMulticore::cpuStackTop(),
        []()
        {
            TimerTickHandler::handleTimerTick();

            ((char*)ArchCommon::getFBPtr())[1 + SMP::currentCpuId() * 2] =
                ((currentThread->console_color << 4) | CONSOLECOLOR::BRIGHT_WHITE);

            ArchInterrupts::endOfInterrupt(InterruptVector::APIC_TIMER);
            contextSwitch();
            assert(false);
        });
}

// yield
void int65_handler_swi_yield()
{
    debugAdvanced(A_INTERRUPTS, "Interrupt %u called on CPU %zu\n", InterruptVector::YIELD, SMP::currentCpuId());

    ArchCommon::callWithStack(ArchMulticore::cpuStackTop(),
        []()
        {
            Scheduler::instance()->schedule();
            ((char*)ArchCommon::getFBPtr())[1 + SMP::currentCpuId()*2] =
                ((currentThread->console_color << 4) |
                 CONSOLECOLOR::BRIGHT_WHITE);

            ArchInterrupts::endOfInterrupt(InterruptVector::YIELD);
            contextSwitch();
            assert(false);
        });
}

void pageFaultHandler(uint32_t address, uint32_t error, uint32_t ip)
{
  assert(!(error & FLAG_PF_RSVD) && "Reserved bit set in page table entry");
  PageFaultHandler::enterPageFault(address,
                                   ip,
                                   error & FLAG_PF_USER,
                                   error & FLAG_PF_PRESENT,
                                   error & FLAG_PF_RDWR,
                                   error & FLAG_PF_INSTR_FETCH);
  if (currentThread->switch_to_userspace_)
    contextSwitch();
  else
    asm volatile ("movl %%cr3, %%eax; movl %%eax, %%cr3;" ::: "%eax");
}

void int90_handler_halt_cpu()
{
    while (assert_print_lock.test_and_set(eastl::memory_order_acquire));
    debug(A_INTERRUPTS, "IRQ %u called, cpu %zu halting\n", InterruptVector::IPI_HALT_CPU, SMP::currentCpuId());
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

void int91_handler_APIC_error()
{
    auto error = cpu_lapic->readRegister<Apic::Register::ERROR_STATUS>();
    debugAlways(APIC, "Internal APIC error: %x\n", *(uint32_t*)&error);

    assert(!"Internal APIC error");

    cpu_lapic->writeRegister<Apic::Register::ERROR_STATUS>({});
}

void int100_handler_APIC_spurious()
{
    debug(A_INTERRUPTS, "IRQ %u called on CPU %zu, spurious APIC interrupt\n", InterruptVector::APIC_SPURIOUS, SMP::currentCpuId());
}

void int101_handler_cpu_fcall()
{
    debug(A_INTERRUPTS, "IRQ %u called by CPU %zu\n", InterruptVector::IPI_REMOTE_FCALL, SMP::currentCpuId());

    auto funcdata = current_cpu.fcall_queue.takeAll();
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

  auto ret = Syscall::syscallException(currentThread->user_registers_->eax,
                                       currentThread->user_registers_->ebx,
                                       currentThread->user_registers_->ecx,
                                       currentThread->user_registers_->edx,
                                       currentThread->user_registers_->esi,
                                       currentThread->user_registers_->edi);
  currentThread->user_registers_->eax = ret;


  ArchInterrupts::disableInterrupts();
  currentThread->switch_to_userspace_ = 1;
  currentThreadRegisters = currentThread->user_registers_.get();
  contextSwitch();
}

void errorHandler(size_t num, size_t eip, size_t cs, size_t spurious)
{
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
  debug(CPU_ERROR, "Instruction Pointer: %zx, Userspace: %d - currentThread: %p %zd" ":%s, switch_to_userspace_: %d\n",
        eip, userspace, currentThread,
        currentThread ? currentThread->getTID() : -1, currentThread ? currentThread->getName() : 0,
        currentThread ? currentThread->switch_to_userspace_ : -1);

  const Stabs2DebugInfo* deb = kernel_debug_info;
  assert(currentThread && "there should be no fault before there is a current thread");
  assert(currentThread->kernel_registers_ && "every thread needs kernel registers");
  const ArchThreadRegisters* registers_ = currentThread->kernel_registers_.get();
  if (userspace)
  {
    assert(currentThread->loader_ && "User Threads need to have a Loader");
    assert(currentThread->user_registers_ && (currentThread->user_registers_->cr3 == currentThread->kernel_registers_->cr3 &&
           "User and Kernel CR3 register values differ, this most likely is a bug!"));
    deb = currentThread->loader_->getDebugInfos();
    registers_ = currentThread->user_registers_.get();
  }
  if(deb && registers_->eip)
  {
    debug(CPU_ERROR, "This Fault was probably caused by:");
    deb->printCallInformation(registers_->eip);
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
