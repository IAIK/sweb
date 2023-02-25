#include "InterruptUtils.h"

#include "ArchSerialInfo.h"
#include "BDManager.h"
#include "ArchMemory.h"
#include "ArchThreads.h"
#include "ArchCommon.h"
#include "ArchMulticore.h"
#include "kprintf.h"
#include "Scheduler.h"
#include "Console.h"

#include "SerialManager.h"
#include "KeyboardManager.h"
#include "ArchInterrupts.h"
#include "backtrace.h"

#include "ErrorHandlers.h"
#include "Loader.h"
#include "PageFaultHandler.h"
#include "Stabs2DebugInfo.h"
#include "Syscall.h"
#include "Thread.h"
#include "TimerTickHandler.h"
#include "offsets.h"
#include "paging-definitions.h"

#include "8259.h"


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


extern "C" void arch_dummyHandler();
extern "C" void arch_dummyHandlerMiddle();

IDTR InterruptUtils::idtr;
InterruptDescriptorTable InterruptUtils::idt;

void InterruptUtils::initialise()
{
  new (&idt) InterruptDescriptorTable{};
  idt.idtr().load();

  if (A_INTERRUPTS & OUTPUT_ENABLED & OUTPUT_ADVANCED)
  {
      for (uint32 i = 0; i < idt.entries.size(); ++i)
      {
          debug(A_INTERRUPTS,
                "%x -- offset = %p, offset_low = %x, offset_high = %x, present = %x, "
                "segment_selector = %x, type = %x, dpl = %x\n",
                i, handlers[i].handler_func, idt.entries[i].offset_low,
                idt.entries[i].offset_high, idt.entries[i].present,
                idt.entries[i].segment_selector, idt.entries[i].type, idt.entries[i].dpl);
      }
  }
}

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

extern "C" void arch_irqHandler_0();
extern "C" void irqHandler_0()
{
  debugAdvanced(A_INTERRUPTS, "[CPU %zu] IRQ 0 called\n", SMP::currentCpuId());

  ArchInterrupts::startOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 0); // wrong irq number (+32)

  ArchCommon::callWithStack(
      ArchMulticore::cpuStackTop(),
      []()
      {
          TimerTickHandler::handleTimerTick();

          ((char*)ArchCommon::getFBPtr())[1 + SMP::currentCpuId()*2] =
              ((currentThread->console_color << 4) | CONSOLECOLOR::BRIGHT_WHITE);

          ArchInterrupts::endOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 0);
          contextSwitch();
          assert(false);
      });
}

extern "C" void irqHandler_127()
{
    debugAdvanced(A_INTERRUPTS, "[CPU %zu] IRQ 127 called\n", SMP::currentCpuId());

    ArchInterrupts::startOfInterrupt(127);

    ArchCommon::callWithStack(
        ArchMulticore::cpuStackTop(),
        []()
        {
            TimerTickHandler::handleTimerTick();

            ((char*)ArchCommon::getFBPtr())[1 + SMP::currentCpuId() * 2] =
                ((currentThread->console_color << 4) | CONSOLECOLOR::BRIGHT_WHITE);

            ArchInterrupts::endOfInterrupt(127);
            contextSwitch();
            assert(false);
        });
}

extern "C" void arch_irqHandler_65();
extern "C" void irqHandler_65()
{
    if (A_INTERRUPTS & OUTPUT_ADVANCED)
        debug(A_INTERRUPTS, "Interrupt 65 called on CPU %zu\n", SMP::currentCpuId());

    ArchCommon::callWithStack(ArchMulticore::cpuStackTop(),
        []()
        {
            Scheduler::instance()->schedule();
            ((char*)ArchCommon::getFBPtr())[1 + SMP::currentCpuId()*2] =
                ((currentThread->console_color << 4) |
                 CONSOLECOLOR::BRIGHT_WHITE);

            contextSwitch();
            assert(false);
        });
}

extern "C" void arch_pageFaultHandler();
extern "C" void pageFaultHandler(uint32 address, uint32 error, uint32 ip)
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

extern "C" void arch_irqHandler_1();
extern "C" void irqHandler_1()
{
  debug(A_INTERRUPTS, "IRQ 1 called on CPU %zu\n", SMP::currentCpuId());
  ArchInterrupts::startOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 1);
  KeyboardManager::instance().serviceIRQ();
  ArchInterrupts::endOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 1);
}

extern "C" void arch_irqHandler_3();
extern "C" void irqHandler_3()
{
  debug(A_INTERRUPTS, "IRQ 3 called on CPU %zu\n", SMP::currentCpuId());
  ArchInterrupts::startOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 3);
  SerialManager::instance().service_irq(3);
  ArchInterrupts::endOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 3);
}

extern "C" void arch_irqHandler_4();
extern "C" void irqHandler_4()
{
  debug(A_INTERRUPTS, "IRQ 4 called on CPU %zu\n", SMP::currentCpuId());
  ArchInterrupts::startOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 4);
  SerialManager::instance().service_irq(4);
  ArchInterrupts::endOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 4);
}

extern "C" void arch_irqHandler_6();
extern "C" void irqHandler_6()
{
  debug(A_INTERRUPTS, "IRQ 6 called on CPU %zu\n", SMP::currentCpuId());
}

extern "C" void arch_irqHandler_9();
extern "C" void irqHandler_9()
{
  debug(A_INTERRUPTS, "IRQ 9 called on CPU %zu\n", SMP::currentCpuId());
  ArchInterrupts::startOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 9);
  BDManager::instance().serviceIRQ(9);
  ArchInterrupts::endOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 9);
}

extern "C" void arch_irqHandler_11();
extern "C" void irqHandler_11()
{
  debug(A_INTERRUPTS, "IRQ 11 called on CPU %zu\n", SMP::currentCpuId());
  ArchInterrupts::startOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 11);
  BDManager::instance().serviceIRQ(11);
  ArchInterrupts::endOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 11);
}

extern "C" void arch_irqHandler_14();
extern "C" void irqHandler_14()
{
  debug(A_INTERRUPTS, "IRQ 14 called on CPU %zu\n", SMP::currentCpuId());
  ArchInterrupts::startOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 14);
  BDManager::instance().serviceIRQ(14);
  ArchInterrupts::endOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 14);
}

extern "C" void arch_irqHandler_15();
extern "C" void irqHandler_15()
{
  debug(A_INTERRUPTS, "IRQ 15 called on CPU %zu\n", SMP::currentCpuId());
  ArchInterrupts::startOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 15);
  BDManager::instance().serviceIRQ(15);
  ArchInterrupts::endOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 15);
}

extern eastl::atomic_flag assert_print_lock;

extern "C" void arch_irqHandler_90();
extern "C" void irqHandler_90()
{
        ArchInterrupts::startOfInterrupt(90);

        while (assert_print_lock.test_and_set(eastl::memory_order_acquire));
        debug(A_INTERRUPTS, "IRQ 90 called, cpu %zu halting\n", SMP::currentCpuId());
        if (currentThread != 0)
        {
                debug(BACKTRACE, "CPU %zu backtrace:\n", SMP::currentCpuId());
                currentThread->printBacktrace(false);
        }
        assert_print_lock.clear(eastl::memory_order_release);

        while(1)
        {
            ArchCommon::halt();
        }

        ArchInterrupts::endOfInterrupt(90);
}

extern "C" void irqHandler_91()
{
    ArchInterrupts::startOfInterrupt(91);

    auto error = cpu_lapic->readRegister<Apic::Register::ERROR_STATUS>();
    debugAlways(APIC, "Internal APIC error: %x\n", *(uint32_t*)&error);

    assert(!"Internal APIC error");

    cpu_lapic->writeRegister<Apic::Register::ERROR_STATUS>({});

    ArchInterrupts::endOfInterrupt(91);
}

extern "C" void arch_irqHandler_100();
extern "C" void irqHandler_100()
{
        // No EOI here!
        debug(A_INTERRUPTS, "IRQ 100 called on CPU %zu, spurious APIC interrupt\n", SMP::currentCpuId());
}

extern "C" void arch_irqHandler_101();
extern "C" void irqHandler_101()
{
    debug(A_INTERRUPTS, "IRQ 101 called by CPU %zu\n", SMP::currentCpuId());
    ArchInterrupts::endOfInterrupt(101); // We can acknowledge int receipt early here

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

extern "C" void arch_syscallHandler();
extern "C" void syscallHandler()
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
  //ArchThreads::printThreadRegisters(currentThread,false);
  contextSwitch();
}

extern const Stabs2DebugInfo* kernel_debug_info;
// extern constexpr const char* errors[];
extern "C" void arch_errorHandler();
extern "C" void errorHandler(size_t num, size_t eip, size_t cs, size_t spurious)
{
  if (spurious)
  {
    assert(num < 128 && "there are only 128 interrupts");
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
  ArchThreadRegisters* registers_ = currentThread->kernel_registers_.get();
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
      contextSwitch();
  }
  else
  {
    currentThread->switch_to_userspace_ = false;
    currentThreadRegisters = currentThread->kernel_registers_.get();
    ArchInterrupts::enableInterrupts();
    debug(CPU_ERROR, "Terminating process...\n");
    currentThread->kill();
  }
}

// #include "ErrorHandlers.h" // error handler definitions and irq forwarding definitions

extern "C" void interruptHandler(size_t interrupt_num,
                                 uint32_t error_code,
                                 ArchThreadRegisters* saved_registers)
{
    debugAdvanced(A_INTERRUPTS, "[CPU %zu] Generic interrupt handler %zu, error: %x\n", SMP::currentCpuId(), interrupt_num, error_code);
    assert(interrupt_num < 256);

    // Interrupts that currently need special handling (e.g. stack switch, ...)
    switch (interrupt_num)
    {
    case Apic::IRQ_VECTOR_OFFSET:
        irqHandler_0();
        return;
    case 65:
        irqHandler_65();
        return;
    case 90:
        irqHandler_90();
        return;
    case 91:
        irqHandler_91();
        return;
    case 100:
        irqHandler_100();
        return;
    case 101:
        irqHandler_101();
        return;
    case 127:
        irqHandler_127();
        return;
    case SYSCALL_INTERRUPT:
        syscallHandler();
        return;
    case 0xE: // pagefault
    {
        uintptr_t pagefault_addr = 0;
        asm volatile("movl %%cr2, %[cr2]\n"
                     : [cr2] "=r"(pagefault_addr));
        pageFaultHandler(pagefault_addr, error_code, saved_registers->eip);
        return;
    }
    }

    if (interrupt_num < 32)
    {
        errorHandler(interrupt_num, saved_registers->eip, saved_registers->cs, 0);
    }
    else
    {
        ArchInterrupts::startOfInterrupt(interrupt_num);
        ArchInterrupts::handleInterrupt(interrupt_num);
        ArchInterrupts::endOfInterrupt(interrupt_num);
    }

    debugAdvanced(A_INTERRUPTS, "Generic interrupt handler %zu end\n", interrupt_num);
}
