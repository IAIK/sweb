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

#include "8259.h"

#define LO_WORD(x) (((uint32)(x)) & 0x0000FFFFULL)
#define HI_WORD(x) ((((uint32)(x)) >> 16) & 0x0000FFFFULL)
#define LO_DWORD(x) (((uint64)(x)) & 0x00000000FFFFFFFFULL)
#define HI_DWORD(x) ((((uint64)(x)) >> 32) & 0x00000000FFFFFFFFULL)

#define TYPE_TRAP_GATE      15 // trap gate, i.e. IF flag is *not* cleared
#define TYPE_INTERRUPT_GATE 14 // interrupt gate, i.e. IF flag *is* cleared

#define DPL_KERNEL_SPACE     0 // kernelspace's protection level
#define DPL_USER_SPACE       3 // userspaces's protection level

#define SYSCALL_INTERRUPT 0x80 // number of syscall interrupt


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
InterruptGateDesc* InterruptUtils::idt;

InterruptGateDesc::InterruptGateDesc(uint64 offset, uint8 dpl) :
  segment_selector(KERNEL_CS),
  ist(0),
  zeros(0),
  type(TYPE_INTERRUPT_GATE),
  zero_1(0),
  dpl(dpl),
  present(1),
  reserved(0)
{
  setOffset(offset);
}

void InterruptGateDesc::setOffset(uint64 offset)
{
  offset_ld_lw = LO_WORD(LO_DWORD( offset ));
  offset_ld_hw = HI_WORD(LO_DWORD( offset ));
  offset_hd =    HI_DWORD(         offset );
}

void IDTR::load()
{
  debug(A_INTERRUPTS, "Loading IDT, base: %zx, limit: %x\n", base, limit);
  asm volatile("lidt (%0) ": :"q" (this));
}

void InterruptUtils::initialise()
{
  uint32 num_handlers = 0;
  for (uint32 i = 0; handlers[i].offset != 0; ++i)
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
    idt[i] = InterruptGateDesc(((size_t)arch_dummyHandler) + i*dummy_handler_sled_size, DPL_KERNEL_SPACE);
  }

  uint32 j = 0;
  while(handlers[j].offset != 0)
  {
    assert(handlers[j].number < num_handlers);
    uint8 dpl = (handlers[j].number == SYSCALL_INTERRUPT) ? DPL_USER_SPACE :
                                                            DPL_KERNEL_SPACE;
    idt[handlers[j].number] = InterruptGateDesc((size_t)handlers[j].offset, dpl);
    ++j;
  }

  if(A_INTERRUPTS & OUTPUT_ENABLED)
  {
    for (uint32 i = 0; i < num_handlers; ++i)
    {
      debug(A_INTERRUPTS,
            "%x -- offset = %p, offset_ld_lw = %x, offset_ld_hw = %x, offset_hd = %x, ist = %x, present = %x, segment_selector = %x, type = %x, dpl = %x\n", i, handlers[i].offset,
            idt[i].offset_ld_lw, idt[i].offset_ld_hw,
            idt[i].offset_hd, idt[i].ist,
            idt[i].present, idt[i].segment_selector,
            idt[i].type, idt[i].dpl);
    }
  }

  idtr.base = (pointer) idt;
  idtr.limit = sizeof(InterruptGateDesc) * num_handlers - 1;
  idtr.load();
}

extern SWEBDebugInfo const *kernel_debug_info;


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
  if (A_INTERRUPTS & OUTPUT_ADVANCED)
      debug(A_INTERRUPTS, "IRQ 0 called by CPU %zu\n", SMP::currentCpuId());
  ArchInterrupts::startOfInterrupt(0);
  ArchCommon::drawHeartBeat();

  Scheduler::instance()->incCpuTicks();

  ArchCommon::callWithStack(ArchMulticore::cpuStackTop(),
    []()
    {
      Scheduler::instance()->schedule();

      ((char*)ArchCommon::getFBPtr())[1 + SMP::currentCpuId()*2] =
          ((currentThread->console_color << 4) |
           CONSOLECOLOR::BRIGHT_WHITE);

      ArchInterrupts::endOfInterrupt(0);
      contextSwitch();
      assert(false);
    });
}

// yield
extern "C" void arch_irqHandler_65();
extern "C" void irqHandler_65()
{
  if (A_INTERRUPTS & OUTPUT_ADVANCED)
      debug(A_INTERRUPTS, "IRQ 65 called by CPU %zu\n", SMP::currentCpuId());
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

extern "C" void errorHandler(size_t num, size_t eip, size_t cs, size_t spurious);
extern "C" void arch_pageFaultHandler();
extern "C" void pageFaultHandler(uint64 address, uint64 error, uint64 ip)
{
  if (address >= USER_BREAK && address < KERNEL_START) { // dirty hack due to qemu invoking the pf handler when accessing non canonical addresses
      auto &regs = *(currentThread->switch_to_userspace_ ? currentThread->user_registers_ : currentThread->kernel_registers_);
      errorHandler(0xd, regs.rip, regs.cs, 0);
      assert(0 && "thread should not survive a GP fault");
  }
  assert(!(error & FLAG_PF_RSVD) && "Reserved bit set in page table entry");

  PageFaultHandler::enterPageFault(address, ip, error & FLAG_PF_USER,
                                   error & FLAG_PF_PRESENT,
                                   error & FLAG_PF_RDWR,
                                   error & FLAG_PF_INSTR_FETCH);
  if (currentThread->switch_to_userspace_)
    contextSwitch();
  else
    asm volatile ("movq %%cr3, %%rax; movq %%rax, %%cr3;" ::: "%rax");
}

extern "C" void arch_irqHandler_1();
extern "C" void irqHandler_1()
{
  ArchInterrupts::startOfInterrupt(1);
  debug(A_INTERRUPTS, "Interrupt vector %u (%x) called\n", 1, 1 + 0x20);
  KeyboardManager::instance()->serviceIRQ( );
  ArchInterrupts::endOfInterrupt(1);
}

extern "C" void arch_irqHandler_3();
extern "C" void irqHandler_3()
{
  kprintfd( "IRQ 3 called\n" );
  ArchInterrupts::startOfInterrupt(3);
  debug(A_INTERRUPTS, "Interrupt vector %u (%x) called\n", 3, 3 + 0x20);
  SerialManager::getInstance()->service_irq( 3 );
  ArchInterrupts::endOfInterrupt(3);
  kprintfd( "IRQ 3 ended\n" );
}

extern "C" void arch_irqHandler_4();
extern "C" void irqHandler_4()
{
  debug(A_INTERRUPTS, "Interrupt vector %u (%x) called\n", 4, 4 + 0x20);
  kprintfd( "IRQ 4 called\n" );
  ArchInterrupts::startOfInterrupt(3);
  SerialManager::getInstance()->service_irq( 4 );
  ArchInterrupts::endOfInterrupt(3);
  kprintfd( "IRQ 4 ended\n" );
}

extern "C" void arch_irqHandler_6();
extern "C" void irqHandler_6()
{
  debug(A_INTERRUPTS, "Interrupt vector %u (%x) called\n", 6, 6 + 0x20);
  kprintfd( "IRQ 6 called\n" );
  kprintfd( "IRQ 6 ended\n" );
}

extern "C" void arch_irqHandler_9();
extern "C" void irqHandler_9()
{
  debug(A_INTERRUPTS, "Interrupt vector %u (%x) called\n", 9, 9 + 0x20);
  kprintfd( "IRQ 9 called\n" );
  ArchInterrupts::startOfInterrupt(9);
  BDManager::getInstance()->serviceIRQ( 9 );
  ArchInterrupts::endOfInterrupt(9);
}

extern "C" void arch_irqHandler_11();
extern "C" void irqHandler_11()
{
  debug(A_INTERRUPTS, "Interrupt vector %u (%x) called by core %zx\n", 11, 11 + 0x20, SMP::currentCpuId());
  kprintfd( "IRQ 11 called\n" );
  ArchInterrupts::startOfInterrupt(11);
  BDManager::getInstance()->serviceIRQ( 11 );
  ArchInterrupts::endOfInterrupt(11);
}

extern "C" void arch_irqHandler_14();
extern "C" void irqHandler_14()
{
  debug(A_INTERRUPTS, "Interrupt vector %u (%u) called by core %zx\n", 14, 14 + 0x20, SMP::currentCpuId());
  //kprintfd( "IRQ 14 called\n" );
  ArchInterrupts::startOfInterrupt(14);
  BDManager::getInstance()->serviceIRQ( 14 );
  ArchInterrupts::endOfInterrupt(14);
}

extern "C" void arch_irqHandler_15();
extern "C" void irqHandler_15()
{
  debug(A_INTERRUPTS, "Interrupt vector %u (%x) called\n", 15, 15 + 0x20);
  //kprintfd( "IRQ 15 called\n" );
  ArchInterrupts::startOfInterrupt(15);
  BDManager::getInstance()->serviceIRQ( 15 );
  ArchInterrupts::endOfInterrupt(15);
}

extern eastl::atomic_flag assert_print_lock;

extern "C" void arch_irqHandler_90();
extern "C" void irqHandler_90()
{
        ArchInterrupts::startOfInterrupt(90 - 0x20);

        while (assert_print_lock.test_and_set(eastl::memory_order_acquire));
        debug(A_INTERRUPTS, "IRQ 90 called, CPU %zu halting\n", SMP::currentCpuId());
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

        ArchInterrupts::endOfInterrupt(90 - 0x20);
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
    ArchInterrupts::endOfInterrupt(101 - 0x20); // We can acknowledge int receipt early here

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
  currentThreadRegisters = currentThread->kernel_registers_;
  ArchInterrupts::enableInterrupts();

  currentThread->user_registers_->rax =
    Syscall::syscallException(currentThread->user_registers_->rax,
                  currentThread->user_registers_->rbx,
                  currentThread->user_registers_->rcx,
                  currentThread->user_registers_->rdx,
                  currentThread->user_registers_->rsi,
                  currentThread->user_registers_->rdi);

  ArchInterrupts::disableInterrupts();
  currentThread->switch_to_userspace_ = 1;
  currentThreadRegisters = currentThread->user_registers_;
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
    currentThreadRegisters = currentThread->kernel_registers_;
    ArchInterrupts::enableInterrupts();
    debug(CPU_ERROR, "Terminating process...\n");
    if (currentThread->user_registers_)
      Syscall::exit(888);
    else
      currentThread->kill();
    assert(false);
  }
}

#include "ErrorHandlers.h" // error handler definitions and irq forwarding definitions
