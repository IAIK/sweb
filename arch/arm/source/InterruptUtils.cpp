/**
 * @file InterruptUtils.cpp
 *
 */

#include "InterruptUtils.h"
#include "new.h"
#include "arch_panic.h"
#include "ports.h"
#include "ArchMemory.h"
#include "ArchThreads.h"
#include "ArchCommon.h"
#include "Console.h"
#include "FrameBufferConsole.h"
#include "Terminal.h"
#include "kprintf.h"
#include "Scheduler.h"
#include "debug_bochs.h"

#include "arch_serial.h"
#include "serial.h"
#include "arch_keyboard_manager.h"
#include "arch_bd_manager.h"
#include "panic.h"

#include "Thread.h"
#include "ArchInterrupts.h"
#include "backtrace.h"

//remove this later
#include "Thread.h"
#include "Loader.h"
#include "Syscall.h"
#include "paging-definitions.h"
//---------------------------------------------------------------------------*/

void InterruptUtils::initialise()
{
}

extern uint32* currentStack;
extern Console* main_console;

extern ArchThreadInfo *currentThreadInfo;
extern Thread *currentThread;

//TODO extern "C" void arch_pageFaultHandler();
void pageFaultHandler(uint32 address, uint32 error)
{
  //--------Start "just for Debugging"-----------
/*
  debug(PM, "[PageFaultHandler] Address: %x, Present: %d, Writing: %d, User: %d, Rsvc: %d - currentThread: %x %d:%s, switch_to_userspace_: %d\n",
      address, error & FLAG_PF_PRESENT, (error & FLAG_PF_RDWR) >> 1, (error & FLAG_PF_USER) >> 2, (error & FLAG_PF_RSVD) >> 3, currentThread, currentThread->getPID(),
      currentThread->getName(), currentThread->switch_to_userspace_);

  debug(PM, "[PageFaultHandler] The Pagefault was caused by an %s fetch\n", error & FLAG_PF_INSTR_FETCH ? "instruction" : "operand");

  if (!(error & FLAG_PF_USER))
  {
    // The PF happened in kernel mode? Cool, let's look up the function that caused it.
    // A word of warning: Due to the way the lookup is performed, we may be
    // returned a wrong function name here! Especially routines residing inside
    // ASM- modules are very likely to be detected incorrectly.
    char FunctionName[255];
    pointer StartAddr = get_function_name(currentThread->kernel_arch_thread_info_->eip, FunctionName);

    if (StartAddr)
      debug(PM, "[PageFaultHandler] This pagefault was probably caused by function <%s+%x>\n", FunctionName,
          currentThread->kernel_arch_thread_info_->eip - StartAddr);
  }

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

      page_directory_entry *page_directory = (page_directory_entry *) ArchMemory::getIdentAddressOfPPN(currentThread->loader_->arch_memory_.page_dir_page_);
      uint32 virtual_page = address / PAGE_SIZE;
      uint32 pde_vpn = virtual_page / PAGE_TABLE_ENTRIES;
      uint32 pte_vpn = virtual_page % PAGE_TABLE_ENTRIES;
//      if (page_directory[pde_vpn].pde4k.present)
//      {
//        if (page_directory[pde_vpn].pde4m.use_4_m_pages)
//        {
//          debug(PM, "[PageFaultHandler] Page %d is a 4MiB Page\n", virtual_page);
//          debug(PM, "[PageFaultHandler] Page %d Flags are: writeable:%d, userspace_accessible:%d,\n", virtual_page,
//              page_directory[pde_vpn].pde4m.writeable, page_directory[pde_vpn].pde4m.user_access);
//        }
//        else
//        {
//          page_table_entry *pte_base = (page_table_entry *) ArchMemory::getIdentAddressOfPPN(page_directory[pde_vpn].pde4k.page_table_base_address);
//          debug(PM, "[PageFaultHandler] Page %d is a 4KiB Page\n", virtual_page);
//          debug(PM, "[PageFaultHandler] Page %d Flags are: present:%d, writeable:%d, userspace_accessible:%d,\n", virtual_page,
//            pte_base[pte_vpn].present, pte_base[pte_vpn].writeable, pte_base[pte_vpn].user_access);
//        }
//      }
//      else
//        debug(PM, "[PageFaultHandler] WTF? PDE non-present but Exception present flag was set\n");
    }
    else
    {
      if (address >= 2U*1024U*1024U*1024U)
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
          debug(PM, "[PageFaultHandler] WARNING: This is unusual for addresses above 2Gb, unless you are swapping kernel pages\n");
          debug(PM, "[PageFaultHandler] WARNING: Most likey there is an pointer error somewhere\n");
        }
      }
      else
      {
        //debug(A_INTERRUPTS | PM, "The virtual page we accessed was not mapped to a physical page\n");
        //debug(A_INTERRUPTS | PM, "this is normal and the Loader will propably take care of it now\n");
      }
    }
  }

  ArchThreads::printThreadRegisters(currentThread,0);
  ArchThreads::printThreadRegisters(currentThread,1);

  //--------End "just for Debugging"-----------


  //save previous state on stack of currentThread
  uint32 saved_switch_to_userspace = currentThread->switch_to_userspace_;
  currentThread->switch_to_userspace_ = false;
  currentThreadInfo = currentThread->kernel_arch_thread_info_;
  ArchInterrupts::enableInterrupts();

  //lets hope this Exeption wasn't thrown during a TaskSwitch
  if (! (error & FLAG_PF_PRESENT) && address < 2U*1024U*1024U*1024U && currentThread->loader_)
  {
    currentThread->loader_->loadOnePageSafeButSlow(address); //load stuff
  }
  else
  {
    debug(PM, "[PageFaultHandler] !(error & FLAG_PF_PRESENT): %x, address: %x, loader_: %x\n",
        !(error & FLAG_PF_PRESENT), address < 2U*1024U*1024U*1024U, currentThread->loader_);

    if (!(error & FLAG_PF_USER))
      currentThread->printBacktrace(true);

    if (currentThread->loader_)
      Syscall::exit(9999);
    else
      currentThread->kill();
  }
  ArchInterrupts::disableInterrupts();
  currentThread->switch_to_userspace_ = saved_switch_to_userspace;
  switch (currentThread->switch_to_userspace_)
  {
    case 0:
      break; //we already are in kernel mode
    case 1:
      currentThreadInfo = currentThread->user_arch_thread_info_;
      //TODO arch_switchThreadToUserPageDirChange();
      break; //not reached
    default:
      kpanict((uint8*)"PageFaultHandler: Undefinded switch_to_userspace value\n");
  }*/
}

void arch_uart0_irq_handler()
{
  kprintfd("arch_uart0_irq_handler\n");
  while(1);
}

void arch_uart1_irq_handler()
{
  kprintfd("arch_uart1_irq_handler\n");
  while(1);
}

void arch_keyboard_irq_handler()
{
  extern struct KMI* kmi;
  while (kmi->stat & 0x10)
  {
    KeyboardManager::instance()->serviceIRQ();
  }
}

void arch_mouse_irq_handler()
{
  kprintfd("arch_mouse_irq_handler\n");
  while(1);
}

void arch_timer0_irq_handler()
{
  static uint32 heart_beat_value = 0;
  uint32 *t0mmio = (uint32*)0x13000000;
  if ((t0mmio[REG_INTSTAT] & 0x1) != 0)
  {
    assert(!ArchInterrupts::testIFSet());
    t0mmio[REG_INTCLR] = 1;     /* according to the docs u can write any value */

    const char* clock = "/-\\|";
    ((FrameBufferConsole*)main_console)->consoleSetCharacter(0,0,clock[heart_beat_value],0);
    heart_beat_value = (heart_beat_value + 1) % 4;

    Scheduler::instance()->incTicks();
    Scheduler::instance()->schedule();

  }
}

void arch_swi_irq_handler()
{
  assert(!ArchInterrupts::testIFSet());
  uint32 swi = *((uint32*)((uint32)currentThreadInfo->pc - 4)) & 0xffff;

  if (swi == 0xffff) // yield
  {
    Scheduler::instance()->schedule();
  }
  else
  {
    assert(false);
    currentThread->switch_to_userspace_ = false;
    currentThreadInfo = currentThread->kernel_arch_thread_info_;
    ArchInterrupts::enableInterrupts();
    currentThreadInfo->r0 = Syscall::syscallException(swi, currentThreadInfo->r0, currentThreadInfo->r1,
                                                           currentThreadInfo->r2, currentThreadInfo->r3,
                                                           currentThreadInfo->r4);
    ArchInterrupts::disableInterrupts();
    currentThread->switch_to_userspace_ = true;
    currentThreadInfo =  currentThread->user_arch_thread_info_;
  }
}

#define IRQ(X) picmmio[PIC_IRQ_STATUS] & (1 << X)

extern "C" void exceptionHandler(uint32 type) {
  if (type == ARM4_XRQ_IRQ) {
    uint32* picmmio = (uint32*)0x14000000;

    if (IRQ(0))
      arch_swi_irq_handler();
    if (IRQ(1))
      arch_uart0_irq_handler();
    if (IRQ(2))
      arch_uart1_irq_handler();
    if (IRQ(3))
      arch_keyboard_irq_handler();
    if (IRQ(4))
      arch_mouse_irq_handler();
    if (IRQ(5))
      arch_timer0_irq_handler();
    // 6-10 and 22-28 not implemented
  }
  else if (type == ARM4_XRQ_SWINT) {
    arch_swi_irq_handler();
  }

  else if (type != ARM4_XRQ_IRQ && type != ARM4_XRQ_FIQ && type != ARM4_XRQ_SWINT) {
    kprintfd("\nCPU Fault type = %x\n",type);
    ArchThreads::printThreadRegisters(currentThread,0);
    ArchThreads::printThreadRegisters(currentThread,1);
    currentThread->switch_to_userspace_ = false;
    currentThreadInfo = currentThread->kernel_arch_thread_info_;
    currentThread->kill();
    for(;;);
  }
}
