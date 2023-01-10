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

#include "8259.h"

#define DPL_KERNEL_SPACE     0 // kernelspace's protection level
#define DPL_USER_SPACE       3 // userspaces's protection level

extern "C" void arch_dummyHandler();
extern "C" void arch_dummyHandlerMiddle();

IDTR InterruptUtils::idtr;
InterruptGateDesc* InterruptUtils::idt;

extern "C" uint64_t generated_idt_vector_table[256];

void InterruptUtils::initialise()
{
  uint32 num_handlers = 0;
  for (uint32 i = 0; handlers[i].handler_func != 0; ++i)
  {
    num_handlers = Max(handlers[i].number, num_handlers);
  }
  num_handlers += 1;
  idt = new InterruptGateDesc[num_handlers];
  size_t dummy_handler_sled_size = (((size_t) arch_dummyHandlerMiddle) - (size_t) arch_dummyHandler);
  assert((dummy_handler_sled_size % 128) == 0 && "cannot handle weird padding in the kernel binary");
  dummy_handler_sled_size /= 128;

  for (uint32 i = 0; i < num_handlers; ++i)
  {
      idt[i] = InterruptGateDesc((handler_func_t)(((uintptr_t)arch_dummyHandler) + i*dummy_handler_sled_size), DPL_KERNEL_SPACE);
  }

  uint32 j = 0;
  while(handlers[j].handler_func)
  {
      auto num = handlers[j].number;
      assert(num < num_handlers);
      auto handler_func = handlers[j].handler_func;
      uint8 dpl =
          (num == SYSCALL_INTERRUPT) ? DPL_USER_SPACE : DPL_KERNEL_SPACE;

      handler_func = (handler_func_t)generated_idt_vector_table[num];

      idt[num] = InterruptGateDesc(handler_func, dpl);
      ++j;
  }

  if(A_INTERRUPTS & OUTPUT_ENABLED)
  {
    for (uint32 i = 0; i < num_handlers; ++i)
    {
      debug(A_INTERRUPTS,
            "%x -- offset = %p, ist = %x, present = %x, segment_selector = %x, type = %x, dpl = %x\n",
            i,
            idt[i].offset(),
            idt[i].ist,
            idt[i].present, idt[i].segment_selector,
            idt[i].type, idt[i].dpl);
    }
  }

  idtr.base = (pointer) idt;
  idtr.limit = sizeof(InterruptGateDesc) * num_handlers - 1;
  idtr.load();
}

extern const SWEBDebugInfo* kernel_debug_info;

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

extern "C" void arch_irqHandler_0();
extern "C" void irqHandler_0()
{
    debugAdvanced(A_INTERRUPTS, "[CPU %zu] Interrupt vector %u (ISA IRQ %u) called\n",
          SMP::currentCpuId(), Apic::IRQ_VECTOR_OFFSET + 0, 0);

    ArchInterrupts::startOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 0);

    ArchInterrupts::handleInterrupt(Apic::IRQ_VECTOR_OFFSET + 0);

    ArchCommon::callWithStack(
        ArchMulticore::cpuStackTop(),
        []()
        {
            TimerTickHandler::handleTimerTick();

            ((char*)ArchCommon::getFBPtr())[1 + SMP::currentCpuId() * 2] =
                ((currentThread->console_color << 4) | CONSOLECOLOR::BRIGHT_WHITE);

            ArchInterrupts::endOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 0);
            contextSwitch();
            assert(false);
        });
}

extern "C" void arch_irqHandler_127();
extern "C" void irqHandler_127()
{
    debugAdvanced(A_INTERRUPTS, "[CPU %zu] Interrupt vector %u called\n",
                  SMP::currentCpuId(), 127);

    ArchInterrupts::startOfInterrupt(127);

    ArchInterrupts::handleInterrupt(127);

    ArchCommon::callWithStack(ArchMulticore::cpuStackTop(), []()
        {
            TimerTickHandler::handleTimerTick();

            ((char*)ArchCommon::getFBPtr())[1 + SMP::currentCpuId()*2] =
                ((currentThread->console_color << 4) |
                 CONSOLECOLOR::BRIGHT_WHITE);

            ArchInterrupts::endOfInterrupt(127);
            contextSwitch();
            assert(false);
        });
}


// yield
extern "C" void arch_irqHandler_65();
extern "C" void irqHandler_65()
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

            contextSwitch();
            assert(false);
        });
}

extern "C" void errorHandler(size_t num, size_t eip, size_t cs, size_t spurious);
extern "C" void arch_pageFaultHandler();
extern "C" void pageFaultHandler(uint64 address, uint64 error, uint64 ip)
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

extern "C" void arch_irqHandler_1();
extern "C" void irqHandler_1()
{
    debugAdvanced(A_INTERRUPTS, "[CPU %zu] Interrupt vector %u (ISA IRQ %u) called\n",
          SMP::currentCpuId(), Apic::IRQ_VECTOR_OFFSET + 1, 1);
    ArchInterrupts::startOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 1);
    ArchInterrupts::handleInterrupt(Apic::IRQ_VECTOR_OFFSET + 1);
    ArchInterrupts::endOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 1);
}

extern "C" void arch_irqHandler_3();
extern "C" void irqHandler_3()
{
    debug(A_INTERRUPTS, "[CPU %zu] Interrupt vector %u (ISA IRQ %u) called\n",
          SMP::currentCpuId(), Apic::IRQ_VECTOR_OFFSET + 3, 3);
    ArchInterrupts::startOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 3);
    ArchInterrupts::handleInterrupt(Apic::IRQ_VECTOR_OFFSET + 3);
    ArchInterrupts::endOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 3);
}

extern "C" void arch_irqHandler_4();
extern "C" void irqHandler_4()
{
    debug(A_INTERRUPTS, "[CPU %zu] Interrupt vector %u (ISA IRQ %u) called\n",
          SMP::currentCpuId(), Apic::IRQ_VECTOR_OFFSET + 4, 4);
    ArchInterrupts::startOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 4);
    ArchInterrupts::handleInterrupt(Apic::IRQ_VECTOR_OFFSET + 4);
    ArchInterrupts::endOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 4);
}

extern "C" void arch_irqHandler_6();
extern "C" void irqHandler_6()
{
    debug(A_INTERRUPTS, "[CPU %zu] Interrupt vector %u (ISA IRQ %u) called\n",
          SMP::currentCpuId(), Apic::IRQ_VECTOR_OFFSET + 6, 6);
    ArchInterrupts::startOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 6);
    ArchInterrupts::handleInterrupt(Apic::IRQ_VECTOR_OFFSET + 6);
    ArchInterrupts::endOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 6);
}

extern "C" void arch_irqHandler_9();
extern "C" void irqHandler_9()
{
    debug(A_INTERRUPTS, "[CPU %zu] Interrupt vector %u (ISA IRQ %u) called\n",
          SMP::currentCpuId(), Apic::IRQ_VECTOR_OFFSET + 9, 9);
    kprintfd("IRQ 9 called\n");
    ArchInterrupts::startOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 9);
    ArchInterrupts::handleInterrupt(Apic::IRQ_VECTOR_OFFSET + 9);
    ArchInterrupts::endOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 9);
}

extern "C" void arch_irqHandler_11();
extern "C" void irqHandler_11()
{
    debug(A_INTERRUPTS, "[CPU %zu] Interrupt vector %u (ISA IRQ %u) called\n",
          SMP::currentCpuId(), Apic::IRQ_VECTOR_OFFSET + 11, 11);
    ArchInterrupts::startOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 11);
    ArchInterrupts::handleInterrupt(Apic::IRQ_VECTOR_OFFSET + 11);
    ArchInterrupts::endOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 11);
}

extern "C" void arch_irqHandler_14();
extern "C" void irqHandler_14()
{
    debugAdvanced(A_INTERRUPTS, "[CPU %zu] Interrupt vector %u (ISA IRQ %u) called\n",
                  SMP::currentCpuId(), Apic::IRQ_VECTOR_OFFSET + 14, 14);
    ArchInterrupts::startOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 14);
    ArchInterrupts::handleInterrupt(Apic::IRQ_VECTOR_OFFSET + 14);
    ArchInterrupts::endOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 14);
}

extern "C" void arch_irqHandler_15();
extern "C" void irqHandler_15()
{
    debug(A_INTERRUPTS, "[CPU %zu] Interrupt vector %u (ISA IRQ %u) called\n",
          SMP::currentCpuId(), Apic::IRQ_VECTOR_OFFSET + 15, 15);
    ArchInterrupts::startOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 15);
    ArchInterrupts::handleInterrupt(Apic::IRQ_VECTOR_OFFSET + 15);
    ArchInterrupts::endOfInterrupt(Apic::IRQ_VECTOR_OFFSET + 15);
}

extern eastl::atomic_flag assert_print_lock;

extern "C" void arch_irqHandler_90();
extern "C" void irqHandler_90()
{
        ArchInterrupts::startOfInterrupt(90);

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

        ArchInterrupts::endOfInterrupt(90);
}

extern "C" void arch_irqHandler_91();
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
        debug(A_INTERRUPTS, "IRQ 100 called by CPU %zu, spurious APIC interrupt\n", SMP::currentCpuId());
}

extern "C" void arch_irqHandler_101();
extern "C" void irqHandler_101()
{
    debug(A_INTERRUPTS, "IRQ 101 called by CPU %zu\n", SMP::currentCpuId());
    ArchInterrupts::endOfInterrupt(101); // We can acknowledge int receipt early here

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

extern "C" void arch_syscallHandler();
extern "C" void syscallHandler()
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


extern const char* errors[];
extern "C" void arch_errorHandler();
extern "C" void errorHandler(size_t num, size_t rip, size_t cs, size_t spurious)
{
  kprintfd("%zx\n",cs);
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
      contextSwitch();
  }
  else
  {
    currentThread->switch_to_userspace_ = false;
    currentThreadRegisters = currentThread->kernel_registers_.get();
    ArchInterrupts::enableInterrupts();
    debug(CPU_ERROR, "Terminating process...\n");
    if (currentThread->user_registers_)
      Syscall::exit(888);
    else
      currentThread->kill();
    assert(false);
  }
}

extern "C" void genericInterruptHandler(size_t interrupt_num,
                                        [[maybe_unused]] uint64_t error_code,
                                        ArchThreadRegisters* saved_registers)
{
    debugAdvanced(A_INTERRUPTS, "[CPU %zu]Generic interrupt handler %zu, error: %lx\n", SMP::currentCpuId(), interrupt_num,
          error_code);
    assert(interrupt_num < 256);

    switch (interrupt_num)
    {
    case Apic::IRQ_VECTOR_OFFSET:
        irqHandler_0();
        return;
    case 127:
        irqHandler_127();
        return;
    case 65:
        irqHandler_65();
        return;
    case SYSCALL_INTERRUPT:
        syscallHandler();
        return;
    case 101:
        irqHandler_101();
        return;
    case 91:
        irqHandler_91();
        return;
    case 90:
        irqHandler_90();
        return;
    case 0xE:
    {
        uint64_t cr2 = 0;
        asm volatile ("movq %%cr2, %%rax\n"
                      "movq %%rax, %[cr2];"
                      :[cr2]"=g"(cr2)
                      :: "%rax");
        pageFaultHandler(cr2, error_code, saved_registers->rip);
        return;
    }
    }

    if (interrupt_num < 32)
    {
        errorHandler(interrupt_num, saved_registers->rip, saved_registers->cs, 0);
    }
    else
    {
        ArchInterrupts::startOfInterrupt(interrupt_num);
        ArchInterrupts::handleInterrupt(interrupt_num);
        ArchInterrupts::endOfInterrupt(interrupt_num);
    }

    debugAdvanced(A_INTERRUPTS, "Generic interrupt handler %zu end\n", interrupt_num);
}

#include "ErrorHandlers.h" // error handler definitions and irq forwarding definitions
