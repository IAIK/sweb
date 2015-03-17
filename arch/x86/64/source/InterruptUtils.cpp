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
#include "backtrace.h"

//remove this later
#include "Thread.h"
#include "Loader.h"
#include "Syscall.h"
#include "paging-definitions.h"

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

struct GateDesc
{
  uint16 offset_ld_lw : 16;     // low word / low dword of handler entry point's address
  uint16 segment_selector : 16; // (code) segment the handler resides in
  uint8 ist       : 3;     // interrupt stack table index
  uint8 zeros     : 5;     // set to zero
  uint8 type      : 4;     // set to TYPE_TRAP_GATE or TYPE_INTERRUPT_GATE
  uint8 zero_1    : 1;     // unsued - set to zero
  uint8 dpl       : 2;     // descriptor protection level
  uint8 present   : 1;     // present- flag - set to 1
  uint16 offset_ld_hw : 16;     // high word / low dword of handler entry point's address
  uint32 offset_hd : 32;        // high dword of handler entry point's address
  uint32 reserved : 32;
}__attribute__((__packed__));


extern "C" void arch_dummyHandler();

uint64 InterruptUtils::pf_address;
uint64 InterruptUtils::pf_address_counter;

void InterruptUtils::initialise()
{
  uint32 num_handlers = 0;
  for (uint32 i = 0; handlers[i].offset != 0; ++i)
    num_handlers = handlers[i].number;
  ++num_handlers;
  // allocate some memory for our handlers
  GateDesc *interrupt_gates = new GateDesc[num_handlers];

  uint32 j = 0;
  for (uint32 i = 0; i < num_handlers; ++i)
  {
    while (handlers[j].number < i && handlers[j].offset != 0)
      ++j;
    interrupt_gates[i].offset_ld_lw = LO_WORD(LO_DWORD((handlers[j].number == i && handlers[j].offset != 0) ? handlers[j].offset : arch_dummyHandler));
    interrupt_gates[i].offset_ld_hw = HI_WORD(LO_DWORD((handlers[j].number == i && handlers[j].offset != 0) ? handlers[j].offset : arch_dummyHandler));
    interrupt_gates[i].offset_hd = HI_DWORD((handlers[j].number == i && handlers[j].offset != 0) ? handlers[j].offset : arch_dummyHandler);
    interrupt_gates[i].ist = 0; // we could provide up to 7 different indices here - 0 means legacy stack switching
    interrupt_gates[i].present = 1;
    interrupt_gates[i].segment_selector = KERNEL_CS;
    interrupt_gates[i].type = TYPE_INTERRUPT_GATE;
    interrupt_gates[i].zero_1 = 0;
    interrupt_gates[i].zeros = 0;
    interrupt_gates[i].reserved = 0;
    interrupt_gates[i].dpl = ((i == SYSCALL_INTERRUPT && handlers[j].number == i) ? DPL_USER_SPACE : DPL_KERNEL_SPACE);
    debug(A_INTERRUPTS,
        "%x -- offset = %x, offset_ld_lw = %x, offset_ld_hw = %x, offset_hd = %x, ist = %x, present = %x, segment_selector = %x, type = %x, dpl = %x\n", i, handlers[i].offset,
        interrupt_gates[i].offset_ld_lw, interrupt_gates[i].offset_ld_hw,
        interrupt_gates[i].offset_hd, interrupt_gates[i].ist,
        interrupt_gates[i].present, interrupt_gates[i].segment_selector,
        interrupt_gates[i].type, interrupt_gates[i].dpl);
  }
  IDTR idtr;

  idtr.base = (pointer) interrupt_gates;
  idtr.limit = sizeof(GateDesc) * num_handlers - 1;
  lidt(&idtr);
  pf_address = 0xdeadbeef;
  pf_address_counter = 0;
}

void InterruptUtils::lidt(IDTR *idtr)
{
  asm volatile("lidt (%0) ": :"q" (idtr));
}

void InterruptUtils::countPageFault(uint64 address)
{
  if (address == pf_address)
  {
    pf_address_counter++;
  }
  else
  {
    pf_address = address;
  }
  if (pf_address_counter >= 10)
  {
    kprintfd("same pagefault for 10 times in a row. most likely you have an error in your code\n");
    asm("hlt");
  }
}

#define ERROR_HANDLER(x,msg) extern "C" void arch_errorHandler_##x(); \
  extern "C" void errorHandler_##x () \
  {\
    currentThread->switch_to_userspace_ = false;\
    currentThreadInfo = currentThread->kernel_arch_thread_info_;\
    kprintfd("\nCPU Fault " #msg "\n\n%s", intel_manual);\
    asm("hlt");\
    kprintf("\nCPU Fault " #msg "\n\n%s", intel_manual);\
    ArchInterrupts::enableInterrupts();\
    currentThread->kill();\
  }

extern "C" void arch_dummyHandler();
extern "C" void arch_contextSwitch();
extern "C" void dummyHandler()
{
  uint32 saved_switch_to_userspace = currentThread->switch_to_userspace_;
  currentThread->switch_to_userspace_ = 0;
  currentThreadInfo = currentThread->kernel_arch_thread_info_;
  ArchInterrupts::enableInterrupts();
  kprintfd("DUMMY_HANDLER: Spurious INT\n");
  ArchInterrupts::disableInterrupts();
  currentThread->switch_to_userspace_ = saved_switch_to_userspace;
  if (currentThread->switch_to_userspace_)
  {
    currentThreadInfo = currentThread->user_arch_thread_info_;
    arch_contextSwitch();
  }
}

extern ArchThreadInfo *currentThreadInfo;
extern Thread *currentThread;

extern "C" void arch_irqHandler_0();
extern "C" void irqHandler_0()
{
  ArchCommon::drawHeartBeat();

  Scheduler::instance()->incTicks();

  Scheduler::instance()->schedule();

  //kprintfd("irq0: Going to leave irq Handler 0\n");
  ArchInterrupts::EndOfInterrupt(0);
  arch_contextSwitch();
}

extern "C" void arch_irqHandler_65();
extern "C" void irqHandler_65()
{
  Scheduler::instance()->schedule();
  arch_contextSwitch();
}


extern "C" void arch_pageFaultHandler();
extern "C" void pageFaultHandler(uint64 address, uint64 error)
{
  //InterruptUtils::countPageFault(address);
  //--------Start "just for Debugging"-----------

  debug(PM, "[PageFaultHandler] Address: %x, Present: %d, Writing: %d, User: %d, Rsvc: %d - currentThread: %x %d:%s, switch_to_userspace_: %d\n",
      address, error & FLAG_PF_PRESENT, (error & FLAG_PF_RDWR) >> 1, (error & FLAG_PF_USER) >> 2, (error & FLAG_PF_RSVD) >> 3, currentThread, currentThread ? currentThread->getTID() : -1ULL,
      currentThread ? currentThread->getName() : 0, currentThread ? currentThread->switch_to_userspace_ : -1ULL);

  debug(PM, "[PageFaultHandler] The Pagefault was caused by an %s fetch\n", error & FLAG_PF_INSTR_FETCH ? "instruction" : "operand");

  if(!address)
  {
    debug(PM, "[PageFaultHandler] Maybe you're dereferencing a null-pointer!\n");
  }

  if (error)
  {
    if (error & FLAG_PF_PRESENT)
    {
      debug(PM, "[PageFaultHandler] We got a pagefault even though the page mapping is present\n");
      debug(PM, "[PageFaultHandler] %s tried to %s address %x\n", (error & FLAG_PF_USER) ? "A userprogram" : "Some kernel code",
        (error & FLAG_PF_RDWR) ? "write to" : "read from", address);

      ArchMemoryMapping m = ArchMemory::resolveMapping((currentThread && currentThread->loader_) ? currentThread->loader_->arch_memory_.page_map_level_4_ : ((uint64)VIRTUAL_TO_PHYSICAL_BOOT(ArchMemory::getRootOfKernelPagingStructure()) / PAGE_SIZE), address / PAGE_SIZE);

      if (m.pd && m.pd[m.pdi].pt.present)
      {
        if (m.pd[m.pdi].page.size)
        {
          debug(PM, "[PageFaultHandler] Page %d is a 2MiB Page\n", address / PAGE_SIZE);
          debug(PM, "[PageFaultHandler] Page %d Flags are: writeable:%d, userspace_accessible:%d,\n", address / PAGE_SIZE,
                m.pd[m.pdi].page.writeable, m.pd[m.pdi].page.user_access);
        }
        else
        {
          debug(PM, "[PageFaultHandler] Page %d is a 4KiB Page\n", address / PAGE_SIZE);
          debug(PM, "[PageFaultHandler] Page %d Flags are: present:%d, writeable:%d, userspace_accessible:%d,\n", address / PAGE_SIZE,
                m.pt[m.pti].present, m.pt[m.pti].writeable, m.pt[m.pti].user_access);
        }
      }
      else
        debug(PM, "[PageFaultHandler] WTF? PDE non-present but Exception present flag was set\n");
    }
    else
    {
      if (address >= 0xFFFFFFFF00000000ULL)
      {
        debug(PM, "[PageFaultHandler] The virtual page we accessed was not mapped to a physical page\n");
        if (error & FLAG_PF_USER)
        {
          debug(PM, "[PageFaultHandler] WARNING: Your Userspace Programm tried to read from an unmapped address >2GiB\n");
          debug(PM, "[PageFaultHandler] WARNING: Most likey there is an pointer error somewhere\n");
        }
        else
        {
          // remove this error check if your implementation swaps out kernel pages
          debug(PM, "[PageFaultHandler] WARNING: This is unusual for addresses in upper half, unless you are swapping kernel pages\n");
          debug(PM, "[PageFaultHandler] WARNING: Most likey there is an pointer error somewhere\n");
        }
      }
      else
      {
//        debug(PM, "The virtual page we accessed was not mapped to a physical page\n");
//        debug(PM, "this is normal and the Loader will propably take care of it now\n");
      }
    }
  }

  ArchThreads::printThreadRegisters(currentThread,false);
  //--------End "just for Debugging"-----------


  //save previous state on stack of currentThread
  uint32 saved_switch_to_userspace = currentThread->switch_to_userspace_;
  currentThread->switch_to_userspace_ = 0;
  currentThreadInfo = currentThread->kernel_arch_thread_info_;
  ArchInterrupts::enableInterrupts();

  //lets hope this Exeption wasn't thrown during a TaskSwitch
  if (! (error & FLAG_PF_PRESENT) && address < 0xFFFFFFFF00000000ULL && currentThread->loader_)
  {
    currentThread->loader_->loadOnePageSafeButSlow(address); //load stuff
  }
  else
  {
    debug(PM, "[PageFaultHandler] !(error & FLAG_PF_PRESENT): %x, address: %x, loader_: %x\n",
        !(error & FLAG_PF_PRESENT), address < 0xFFFFFFFF00000000ULL, currentThread->loader_);

    if (!(error & FLAG_PF_USER))
      currentThread->printBacktrace(true);

    if (currentThread->loader_)
      Syscall::exit(9999);
    else
      currentThread->kill();
  }
  ArchInterrupts::disableInterrupts();
  asm volatile ("movq %cr3, %rax; movq %rax, %cr3;");
  currentThread->switch_to_userspace_ = saved_switch_to_userspace;
  if (currentThread->switch_to_userspace_)
  {
    currentThreadInfo = currentThread->user_arch_thread_info_;
    arch_contextSwitch();
  }
}

extern "C" void arch_irqHandler_1();
extern "C" void irqHandler_1()
{
  KeyboardManager::instance()->serviceIRQ( );
  ArchInterrupts::EndOfInterrupt(1);
}

extern "C" void arch_irqHandler_3();
extern "C" void irqHandler_3()
{
  kprintfd( "IRQ 3 called\n" );
  SerialManager::getInstance()->service_irq( 3 );
  ArchInterrupts::EndOfInterrupt(3);
  kprintfd( "IRQ 3 ended\n" );
}

extern "C" void arch_irqHandler_4();
extern "C" void irqHandler_4()
{
  kprintfd( "IRQ 4 called\n" );
  SerialManager::getInstance()->service_irq( 4 );
  ArchInterrupts::EndOfInterrupt(4);
  kprintfd( "IRQ 4 ended\n" );
}

extern "C" void arch_irqHandler_6();
extern "C" void irqHandler_6()
{
  kprintfd( "IRQ 6 called\n" );
  kprintfd( "IRQ 6 ended\n" );
}

extern "C" void arch_irqHandler_9();
extern "C" void irqHandler_9()
{
  kprintfd( "IRQ 9 called\n" );
  BDManager::getInstance()->serviceIRQ( 9 );
  ArchInterrupts::EndOfInterrupt(9);
}

extern "C" void arch_irqHandler_11();
extern "C" void irqHandler_11()
{
  kprintfd( "IRQ 11 called\n" );
  BDManager::getInstance()->serviceIRQ( 11 );
  ArchInterrupts::EndOfInterrupt(11);
}

extern "C" void arch_irqHandler_14();
extern "C" void irqHandler_14()
{
  //kprintfd( "IRQ 14 called\n" );
  BDManager::getInstance()->serviceIRQ( 14 );
  ArchInterrupts::EndOfInterrupt(14);
}

extern "C" void arch_irqHandler_15();
extern "C" void irqHandler_15()
{
  //kprintfd( "IRQ 15 called\n" );
  BDManager::getInstance()->serviceIRQ( 15 );
  ArchInterrupts::EndOfInterrupt(15);
}

extern "C" void arch_syscallHandler();
extern "C" void syscallHandler()
{
  currentThread->switch_to_userspace_ = 0;
  currentThreadInfo = currentThread->kernel_arch_thread_info_;
  ArchInterrupts::enableInterrupts();

  currentThread->user_arch_thread_info_->rax =
    Syscall::syscallException(currentThread->user_arch_thread_info_->rdi,
                  currentThread->user_arch_thread_info_->rsi,
                  currentThread->user_arch_thread_info_->rdx,
                  currentThread->user_arch_thread_info_->rcx,
                  currentThread->user_arch_thread_info_->r8,
                  currentThread->user_arch_thread_info_->r9);

  ArchInterrupts::disableInterrupts();
  currentThread->switch_to_userspace_ = 1;
  currentThreadInfo =  currentThread->user_arch_thread_info_;
  arch_contextSwitch();
}

#include "ErrorHandlers.h" // error handler definitions and irq forwarding definitions

