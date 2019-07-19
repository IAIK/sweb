#include "InterruptUtils.h"

#include "ArchSerialInfo.h"
#include "BDManager.h"
#include "ArchMemory.h"
#include "ArchThreads.h"
#include "ArchCommon.h"
#include "ArchMulticore.h"
#include "kprintf.h"
#include "Scheduler.h"

#include "SerialManager.h"
#include "KeyboardManager.h"
#include "ArchInterrupts.h"
#include "backtrace.h"

#include "Thread.h"
#include "Loader.h"
#include "Syscall.h"
#include "paging-definitions.h"
#include "offsets.h"
#include "PageFaultHandler.h"
#include "Stabs2DebugInfo.h"

#include "8259.h"

#define LO_WORD(x) (((uint32)(x)) & 0x0000FFFF)
#define HI_WORD(x) ((((uint32)(x)) >> 16) & 0x0000FFFF)

enum
{
        TYPE_TASK_GATE = 0x5,
        TYPE_INTERRUPT_GATE_16 = 0x6, // interrupt gate, i.e. IF flag *is* cleared
        TYPE_TRAP_GATE_16 = 0x7, // trap gate, i.e. IF flag is *not* cleared
        TYPE_INTERRUPT_GATE_32 = 0xE,// interrupt gate, i.e. IF flag *is* cleared
        TYPE_TRAP_GATE_32 = 0xF, // trap gate, i.e. IF flag is *not* cleared
};

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

struct GateDesc
{
  uint16 offset_low;       // low word of handler entry point's address
  uint16 segment_selector; // (code) segment the handler resides in
  uint8 reserved  : 5;     // reserved. set to zero
  uint8 zeros     : 3;     // set to zero
  uint8 type      : 3;     // set to TYPE_TRAP_GATE or TYPE_INTERRUPT_GATE
  uint8 gate_size : 1;     // set to GATE_SIZE_16_BIT or GATE_SIZE_32_BIT
  uint8 unused    : 1;     // unsued - set to zero
  uint8 dpl       : 2;     // descriptor protection level
  uint8 present   : 1;     // present- flag - set to 1
  uint16 offset_high;      // high word of handler entry point's address
}__attribute__((__packed__));

extern "C" void arch_dummyHandler();
extern "C" void arch_dummyHandlerMiddle();

IDTR InterruptUtils::idtr;
InterruptGateDesc* InterruptUtils::idt;

InterruptGateDesc::InterruptGateDesc(size_t offset, uint8 dpl) :
  segment_selector(KERNEL_CS),
  unused(0),
  type(TYPE_INTERRUPT_GATE_32),
  zero_1(0),
  dpl(dpl),
  present(1)
{
  setOffset(offset);
}

void InterruptGateDesc::setOffset(size_t offset)
{
  offset_low = LO_WORD(offset);
  offset_high = HI_WORD(offset);
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
  // allocate some memory for our handlers
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
                        "%x -- offset = %p, offset_low = %x, offset_high = %x, present = %x, segment_selector = %x, type = %x, dpl = %x\n", i, handlers[i].offset,
                        idt[i].offset_low, idt[i].offset_high,
                        idt[i].present, idt[i].segment_selector,
                        idt[i].type, idt[i].dpl);
          }
  }

  idtr.base = (pointer) idt;
  idtr.limit = sizeof(InterruptGateDesc) * num_handlers - 1;
  idtr.load();
}


extern "C" void arch_irqHandler_0();
extern "C" void arch_contextSwitch();
extern "C" void irqHandler_0()
{
  debug(A_INTERRUPTS, "IRQ 0 called by CPU %zx\n", ArchMulticore::getCpuID());
  ArchInterrupts::startOfInterrupt(0);
  ArchCommon::drawHeartBeat();

  Scheduler::instance()->incTicks();

  Scheduler::instance()->schedule();

  ArchInterrupts::endOfInterrupt(0);
  arch_contextSwitch();
  assert(false);
}

extern "C" void arch_irqHandler_65();
extern "C" void irqHandler_65()
{
  Scheduler::instance()->schedule();
  // kprintfd("irq65: Going to leave int Handler 65 to user\n");
  arch_contextSwitch();
}

extern "C" void arch_pageFaultHandler();
extern "C" void pageFaultHandler(uint32 address, uint32 error, uint32 ip)
{
  PageFaultHandler::enterPageFault(address, ip, error & FLAG_PF_USER,
                                   error & FLAG_PF_PRESENT,
                                   error & FLAG_PF_RDWR,
                                   error & FLAG_PF_INSTR_FETCH);
  if (currentThread->switch_to_userspace_)
    arch_contextSwitch();
  else
    asm volatile ("movl %%cr3, %%eax; movl %%eax, %%cr3;" ::: "%eax");
}

extern "C" void arch_irqHandler_1();
extern "C" void irqHandler_1()
{
  ArchInterrupts::startOfInterrupt(1);
  KeyboardManager::instance()->serviceIRQ();
  ArchInterrupts::endOfInterrupt(1);
}

extern "C" void arch_irqHandler_3();
extern "C" void irqHandler_3()
{
  kprintfd("IRQ 3 called\n");
  ArchInterrupts::startOfInterrupt(3);
  SerialManager::getInstance()->service_irq(3);
  kprintfd("IRQ 3 ended\n");
  ArchInterrupts::endOfInterrupt(3);
}

extern "C" void arch_irqHandler_4();
extern "C" void irqHandler_4()
{
  kprintfd("IRQ 4 called\n");
  ArchInterrupts::startOfInterrupt(4);
  SerialManager::getInstance()->service_irq(4);
  kprintfd("IRQ 4 ended\n");
  ArchInterrupts::endOfInterrupt(4);
}

extern "C" void arch_irqHandler_6();
extern "C" void irqHandler_6()
{
  kprintfd("IRQ 6 called\n");
  kprintfd("IRQ 6 ended\n");
}

extern "C" void arch_irqHandler_9();
extern "C" void irqHandler_9()
{
  kprintfd("IRQ 9 called\n");
  ArchInterrupts::startOfInterrupt(9);
  BDManager::getInstance()->serviceIRQ(9);
  ArchInterrupts::endOfInterrupt(9);
}

extern "C" void arch_irqHandler_11();
extern "C" void irqHandler_11()
{
  kprintfd("IRQ 11 called\n");
  ArchInterrupts::startOfInterrupt(11);
  BDManager::getInstance()->serviceIRQ(11);
  ArchInterrupts::endOfInterrupt(11);
}

extern "C" void arch_irqHandler_14();
extern "C" void irqHandler_14()
{
  //kprintfd( "IRQ 14 called\n" );
  ArchInterrupts::startOfInterrupt(14);
  BDManager::getInstance()->serviceIRQ(14);
  ArchInterrupts::endOfInterrupt(14);
}

extern "C" void arch_irqHandler_15();
extern "C" void irqHandler_15()
{
  //kprintfd( "IRQ 15 called\n" );
  ArchInterrupts::startOfInterrupt(15);
  BDManager::getInstance()->serviceIRQ(15);
  ArchInterrupts::endOfInterrupt(15);
}


extern "C" void arch_irqHandler_90();
extern "C" void irqHandler_90()
{
        ArchInterrupts::startOfInterrupt(90 - 0x20);
        debug(A_INTERRUPTS, "IRQ 90 called, cpu %zu halting\n", ArchMulticore::getCpuID());
        if (currentThread != 0)
        {
                debug(BACKTRACE, "CPU %zu backtrace:\n", ArchMulticore::getCpuID());
                currentThread->printBacktrace(false);
        }
        while(1)
                asm("hlt\n");
        ArchInterrupts::endOfInterrupt(90 - 0x20);
}

extern "C" void arch_irqHandler_99();
extern "C" void irqHandler_99()
{
        ArchInterrupts::startOfInterrupt(99 - 0x20);  // TODO: Fix APIC interrupt numbering
        debug(A_INTERRUPTS, "IRQ 99 called, performing TLB shootdown on CPU %zx\n", ArchMulticore::getCpuID());

        TLBShootdownRequest* shootdown_list = cpu_info.tlb_shootdown_list.exchange(nullptr);

        if(shootdown_list == nullptr)
        {
                debug(A_INTERRUPTS, "TLB shootdown for CPU %zx already handled previously\n", ArchMulticore::getCpuID());
        }

        while(shootdown_list != nullptr)
        {
                debug(A_INTERRUPTS, "CPU %zx performing TLB shootdown for request %zx, addr %zx from CPU %zx, target %zx\n", ArchMulticore::getCpuID(), shootdown_list->request_id, shootdown_list->addr, shootdown_list->orig_cpu, shootdown_list->target);
                assert(shootdown_list->target == ArchMulticore::getCpuID());
                assert(cpu_info.getCpuID() == ArchMulticore::getCpuID());
                assert(cpu_info.lapic.ID() == ArchMulticore::getCpuID());
                assert(cpu_info.lapic.readID() == ArchMulticore::getCpuID());
                ArchMemory::flushLocalTranslationCaches(shootdown_list->addr);

                TLBShootdownRequest* next = shootdown_list->next;
                // Object is invalid as soon as we acknowledge it
                //shootdown_list->ack++;
                assert((shootdown_list->ack & (1 << ArchMulticore::getCpuID())) == 0);
                assert(shootdown_list->orig_cpu != ArchMulticore::getCpuID());

                shootdown_list->ack |= (1 << ArchMulticore::getCpuID());
                assert(shootdown_list != next);
                shootdown_list = next;
        }

        ArchInterrupts::endOfInterrupt(99 - 0x20);
}

extern "C" void arch_irqHandler_100();
extern "C" void irqHandler_100()
{
        // No EOI here!
        debug(A_INTERRUPTS, "IRQ 100 called by CPU %zu, spurious APIC interrupt\n", ArchMulticore::getCpuID());
}



extern "C" void arch_syscallHandler();
extern "C" void syscallHandler()
{
  currentThread->switch_to_userspace_ = 0;
  currentThreadRegisters = currentThread->kernel_registers_;
  ArchInterrupts::enableInterrupts();

  currentThread->user_registers_->eax = Syscall::syscallException(currentThread->user_registers_->eax,
                                                                         currentThread->user_registers_->ebx,
                                                                         currentThread->user_registers_->ecx,
                                                                         currentThread->user_registers_->edx,
                                                                         currentThread->user_registers_->esi,
                                                                         currentThread->user_registers_->edi);

  ArchInterrupts::disableInterrupts();
  currentThread->switch_to_userspace_ = 1;
  currentThreadRegisters = currentThread->user_registers_;
  //ArchThreads::printThreadRegisters(currentThread,false);
  arch_contextSwitch();
}

extern Stabs2DebugInfo const *kernel_debug_info;
extern const char* errors[];
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
  ArchThreadRegisters* registers_ = currentThread->kernel_registers_;
  if (userspace)
  {
    assert(currentThread->loader_ && "User Threads need to have a Loader");
    assert(currentThread->user_registers_ && (currentThread->user_registers_->cr3 == currentThread->kernel_registers_->cr3 &&
           "User and Kernel CR3 register values differ, this most likely is a bug!"));
    deb = currentThread->loader_->getDebugInfos();
    registers_ = currentThread->user_registers_;
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
      arch_contextSwitch();
  }
  else
  {
    currentThread->switch_to_userspace_ = false;
    currentThreadRegisters = currentThread->kernel_registers_;
    ArchInterrupts::enableInterrupts();
    debug(CPU_ERROR, "Terminating process...\n");
    currentThread->kill();
  }
}

#include "ErrorHandlers.h" // error handler definitions and irq forwarding definitions

