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

void restoreThreadRegisters()
{
  /*
    load registers
  */
  currentStack[-1] = currentThreadInfo->pc;
  currentStack[-2] = currentThreadInfo->r12;
  currentStack[-3] = currentThreadInfo->r11;
  currentStack[-4] = currentThreadInfo->r10;
  currentStack[-5] = currentThreadInfo->r9;
  currentStack[-6] = currentThreadInfo->r8;
  currentStack[-7] = currentThreadInfo->r7;
  currentStack[-8] = currentThreadInfo->r6;
  currentStack[-9] = currentThreadInfo->r5;
  currentStack[-10] = currentThreadInfo->r4;
  currentStack[-11] = currentThreadInfo->r3;
  currentStack[-12] = currentThreadInfo->r2;
  currentStack[-13] = currentThreadInfo->r1;
  currentStack[-14] = currentThreadInfo->r0;
  currentStack[-15] = currentThreadInfo->cpsr;
  /* switch into system mode restore hidden registers then switch back */
  asm("mrs r0, cpsr \n\
     bic r0, r0, #0x1f \n\
     orr r0, r0, #0x1f \n\
     msr cpsr, r0 \n\
     mov sp, %[sp] \n\
     mov lr, %[lr] \n\
     bic r0, r0, #0x1f \n\
     orr r0, r0, #0x12 \n\
     msr cpsr, r0 \n\
     " : : [sp]"r" (currentThreadInfo->sp), [lr]"r" (currentThreadInfo->lr));
  /* go back through normal interrupt return process */
}

#include "MountMinix.h"
extern "C" void exceptionHandler(uint32 type);
void exceptionHandler(uint32 type) {
  static uint32 heart_beat_value = 0;
  uint32      *t0mmio = (uint32*)0x13000000;
  uint32      swi;

  t0mmio[REG_INTCLR] = 1;

  if (type == ARM4_XRQ_IRQ) {
    //if (picmmio[PIC_IRQ_STATUS] & 0x20) // TODO we should check this
    {
      t0mmio[REG_INTCLR] = 1;     /* according to the docs u can write any value */

      const char* clock = "/-\\|";
      ((FrameBufferConsole*)main_console)->consoleSetCharacter(0,0,clock[heart_beat_value],0);
      heart_beat_value = (heart_beat_value + 1) % 4;

      Scheduler::instance()->incTicks();
      Scheduler::instance()->schedule();

      return;
    }
  }
  /*
    Get SWI argument (index).
  */
  if (type == ARM4_XRQ_SWINT) {
    swi = ((uint32*)((uint32)currentThreadInfo->lr - 4))[0] & 0xffff;

    if (swi == 4) { // yield
      kprintfd("SWI\n");
      /*saveThreadRegisters();

      Scheduler::instance()->schedule();
      restoreThreadRegisters();
      currentStack = (uint32*)(currentThread->getStackStartPointer() & ~0xF);
*/
      return;
    }
    return;
  }

  if (type != ARM4_XRQ_IRQ && type != ARM4_XRQ_FIQ && type != ARM4_XRQ_SWINT) {
    /*
      Ensure, the exception return code is correctly handling LR with the
      correct offset. I am using the same return for everything except SWI,
      which requires that LR not be offset before return.
    */
    currentThread->switch_to_userspace_ = false;
    currentThreadInfo = currentThread->kernel_arch_thread_info_;
    currentThread->kill();
    kprintfd("\nCPU Fault type = %x\n",type);
    ArchThreads::printThreadRegisters(currentThread,0);
    for(;;);
  }

  return;
}

extern ArchThreadInfo *currentThreadInfo;
extern Thread *currentThread;

#define IRQ_HANDLER(x) extern "C" void arch_irqHandler_##x(); \
  extern "C" void irqHandler_##x ()  {  \
    kprintfd("IRQ_HANDLER: Spurious IRQ " #x "\n"); \
    kprintf("IRQ_HANDLER: Spurious IRQ " #x "\n"); \
    ArchInterrupts::EndOfInterrupt(x); \
  }; \

//TODO extern "C" void arch_irqHandler_0();
//TODO extern "C" void arch_switchThreadKernelToKernel();
//TODO extern "C" void arch_switchThreadKernelToKernelPageDirChange();
//TODO extern "C" void arch_switchThreadToUserPageDirChange();
extern "C" void irqHandler_0()
{

}

//TODO extern "C" void arch_irqHandler_65();
extern "C" void irqHandler_65()
{
  uint32 ret = Scheduler::instance()->schedule();
  switch (ret)
  {
    case 0:
      // kprintfd("irq65: Going to leave int Handler 65 to kernel\n");
      //TODO arch_switchThreadKernelToKernelPageDirChange();
    case 1:
      // kprintfd("irq65: Going to leave int Handler 65 to user\n");
      //TODO arch_switchThreadToUserPageDirChange();

    default:
      kprintfd("irq65: Panic in int 65 handler\n");
      for( ; ; ) ;
  }
}


//TODO extern "C" void arch_pageFaultHandler();
extern "C" void pageFaultHandler(uint32 address, uint32 error)
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

//TODO extern "C" void arch_irqHandler_1();
extern "C" void irqHandler_1()
{
  KeyboardManager::getInstance()->serviceIRQ( );
  ArchInterrupts::EndOfInterrupt(1);
}

//TODO extern "C" void arch_irqHandler_3();
extern "C" void irqHandler_3()
{
  kprintfd( "IRQ 3 called\n" );
  SerialManager::getInstance()->service_irq( 3 );
  ArchInterrupts::EndOfInterrupt(3);
  kprintfd( "IRQ 3 ended\n" );
}

//TODO extern "C" void arch_irqHandler_4();
extern "C" void irqHandler_4()
{
  kprintfd( "IRQ 4 called\n" );
  SerialManager::getInstance()->service_irq( 4 );
  ArchInterrupts::EndOfInterrupt(4);
  kprintfd( "IRQ 4 ended\n" );
}

//TODO extern "C" void arch_irqHandler_6();
extern "C" void irqHandler_6()
{
  kprintfd( "IRQ 6 called\n" );
  kprintfd( "IRQ 6 ended\n" );
}

//TODO extern "C" void arch_irqHandler_9();
extern "C" void irqHandler_9()
{
  kprintfd( "IRQ 9 called\n" );
  BDManager::getInstance()->serviceIRQ( 9 );
  ArchInterrupts::EndOfInterrupt(9);
}

//TODO extern "C" void arch_irqHandler_11();
extern "C" void irqHandler_11()
{
  kprintfd( "IRQ 11 called\n" );
  BDManager::getInstance()->serviceIRQ( 11 );
  ArchInterrupts::EndOfInterrupt(11);
}

//TODO extern "C" void arch_irqHandler_14();
extern "C" void irqHandler_14()
{
  //kprintfd( "IRQ 14 called\n" );
  BDManager::getInstance()->serviceIRQ( 14 );
  ArchInterrupts::EndOfInterrupt(14);
}

//TODO extern "C" void arch_irqHandler_15();
extern "C" void irqHandler_15()
{
  //kprintfd( "IRQ 15 called\n" );
  BDManager::getInstance()->serviceIRQ( 15 );
  ArchInterrupts::EndOfInterrupt(15);
}

//TODO extern "C" void arch_syscallHandler();
extern "C" void syscallHandler()
{
  currentThread->switch_to_userspace_ = false;
  currentThreadInfo = currentThread->kernel_arch_thread_info_;
  ArchInterrupts::enableInterrupts();

//  currentThread->user_arch_thread_info_->eax =
//    Syscall::syscallException(currentThread->user_arch_thread_info_->eax,
//                  currentThread->user_arch_thread_info_->ebx,
//                  currentThread->user_arch_thread_info_->ecx,
//                  currentThread->user_arch_thread_info_->edx,
//                  currentThread->user_arch_thread_info_->esi,
//                  currentThread->user_arch_thread_info_->edi);

  ArchInterrupts::disableInterrupts();
  currentThread->switch_to_userspace_ = true;
  currentThreadInfo =  currentThread->user_arch_thread_info_;
  //ArchThreads::printThreadRegisters(currentThread,1);
  //TODO arch_switchThreadToUserPageDirChange();
}

//IRQ_HANDLER(1)
//TODO IRQ_HANDLER(2)
//IRQ_HANDLER(3)
//IRQ_HANDLER(4)
//TODO IRQ_HANDLER(5)
//IRQ_HANDLER(6)
//TODO IRQ_HANDLER(7)
//TODO IRQ_HANDLER(8)
//IRQ_HANDLER(9)
//TODO IRQ_HANDLER(10)
//IRQ_HANDLER(11)
//TODO IRQ_HANDLER(12)
//TODO IRQ_HANDLER(13)
//IRQ_HANDLER(14)
//IRQ_HANDLER(15)

extern "C" void arch_dummyHandler();
extern "C" void arch_errorHandler();
extern "C" void arch_irqHandler();

#define DUMMYHANDLER(X) {X, &arch_dummyHandler},
#define ERRORHANDLER(X) {X, &arch_errorHandler},
#define IRQHANDLER(X) {X + 32, &arch_irqHandler},
InterruptHandlers InterruptUtils::handlers[NUM_INTERRUPT_HANDLERS] = {/*
  ERRORHANDLER(0)
  DUMMYHANDLER(1)
  DUMMYHANDLER(2)
  DUMMYHANDLER(3)
  ERRORHANDLER(4)
  ERRORHANDLER(5)
  ERRORHANDLER(6)
  ERRORHANDLER(7)
  ERRORHANDLER(8)
  ERRORHANDLER(9)
  ERRORHANDLER(10)
  ERRORHANDLER(11)
  ERRORHANDLER(12)
  ERRORHANDLER(13)
  {14, &arch_pageFaultHandler},
  DUMMYHANDLER(15)
  ERRORHANDLER(16)
  ERRORHANDLER(17)
  ERRORHANDLER(18)
  ERRORHANDLER(19)
  DUMMYHANDLER(20)
  DUMMYHANDLER(21)
  DUMMYHANDLER(22)
  DUMMYHANDLER(23)
  DUMMYHANDLER(24)
  DUMMYHANDLER(25)
  DUMMYHANDLER(26)
  DUMMYHANDLER(27)
  DUMMYHANDLER(28)
  DUMMYHANDLER(29)
  DUMMYHANDLER(30)
  DUMMYHANDLER(31)
  IRQHANDLER(0)
  IRQHANDLER(1)
  IRQHANDLER(2)
  IRQHANDLER(3)
  IRQHANDLER(4)
  IRQHANDLER(5)
  IRQHANDLER(6)
  IRQHANDLER(7)
  IRQHANDLER(8)
  IRQHANDLER(9)
  IRQHANDLER(10)
  IRQHANDLER(11)
  IRQHANDLER(12)
  IRQHANDLER(13)
  IRQHANDLER(14)
  IRQHANDLER(15)
  DUMMYHANDLER(48)
  DUMMYHANDLER(49)
  DUMMYHANDLER(50)
  DUMMYHANDLER(51)
  DUMMYHANDLER(52)
  DUMMYHANDLER(53)
  DUMMYHANDLER(54)
  DUMMYHANDLER(55)
  DUMMYHANDLER(56)
  DUMMYHANDLER(57)
  DUMMYHANDLER(58)
  DUMMYHANDLER(59)
  DUMMYHANDLER(60)
  DUMMYHANDLER(61)
  DUMMYHANDLER(62)
  DUMMYHANDLER(63)
  DUMMYHANDLER(64)
  IRQHANDLER(65)
  DUMMYHANDLER(66)
  DUMMYHANDLER(67)
  DUMMYHANDLER(68)
  DUMMYHANDLER(69)
  DUMMYHANDLER(70)
  DUMMYHANDLER(71)
  DUMMYHANDLER(72)
  DUMMYHANDLER(73)
  DUMMYHANDLER(74)
  DUMMYHANDLER(75)
  DUMMYHANDLER(76)
  DUMMYHANDLER(77)
  DUMMYHANDLER(78)
  DUMMYHANDLER(79)
  DUMMYHANDLER(80)
  DUMMYHANDLER(81)
  DUMMYHANDLER(82)
  DUMMYHANDLER(83)
  DUMMYHANDLER(84)
  DUMMYHANDLER(85)
  DUMMYHANDLER(86)
  DUMMYHANDLER(87)
  DUMMYHANDLER(88)
  DUMMYHANDLER(89)
  DUMMYHANDLER(90)
  DUMMYHANDLER(91)
  DUMMYHANDLER(92)
  DUMMYHANDLER(93)
  DUMMYHANDLER(94)
  DUMMYHANDLER(95)
  DUMMYHANDLER(96)
  DUMMYHANDLER(97)
  DUMMYHANDLER(98)
  DUMMYHANDLER(99)
  DUMMYHANDLER(100)
  DUMMYHANDLER(101)
  DUMMYHANDLER(102)
  DUMMYHANDLER(103)
  DUMMYHANDLER(104)
  DUMMYHANDLER(105)
  DUMMYHANDLER(106)
  DUMMYHANDLER(107)
  DUMMYHANDLER(108)
  DUMMYHANDLER(109)
  DUMMYHANDLER(110)// NOT TODO: interrupt number
  DUMMYHANDLER(111)
  DUMMYHANDLER(112)
  DUMMYHANDLER(113)
  DUMMYHANDLER(114)
  DUMMYHANDLER(115)
  DUMMYHANDLER(116)
  DUMMYHANDLER(117)
  DUMMYHANDLER(118)
  DUMMYHANDLER(119)
  DUMMYHANDLER(120)
  DUMMYHANDLER(121)
  DUMMYHANDLER(122)
  DUMMYHANDLER(123)
  DUMMYHANDLER(124)
  DUMMYHANDLER(125)
  DUMMYHANDLER(126)
  DUMMYHANDLER(127)
//  DUMMYHANDLER(128)
  {128, &arch_syscallHandler},
  DUMMYHANDLER(129)
  DUMMYHANDLER(130)
  DUMMYHANDLER(131)
  DUMMYHANDLER(132)
  DUMMYHANDLER(133)
  DUMMYHANDLER(134)
  DUMMYHANDLER(135)
  DUMMYHANDLER(136)
  DUMMYHANDLER(137)
  DUMMYHANDLER(138)
  DUMMYHANDLER(139)
  DUMMYHANDLER(140)
  DUMMYHANDLER(141)
  DUMMYHANDLER(142)
  DUMMYHANDLER(143)
  DUMMYHANDLER(144)
  DUMMYHANDLER(145)
  DUMMYHANDLER(146)
  DUMMYHANDLER(147)
  DUMMYHANDLER(148)
  DUMMYHANDLER(149)
  DUMMYHANDLER(150)
  DUMMYHANDLER(151)
  DUMMYHANDLER(152)
  DUMMYHANDLER(153)
  DUMMYHANDLER(154)
  DUMMYHANDLER(155)
  DUMMYHANDLER(156)
  DUMMYHANDLER(157)
  DUMMYHANDLER(158)
  DUMMYHANDLER(159)
  DUMMYHANDLER(160)
  DUMMYHANDLER(161)
  DUMMYHANDLER(162)
  DUMMYHANDLER(163)
  DUMMYHANDLER(164)
  DUMMYHANDLER(165)
  DUMMYHANDLER(166)
  DUMMYHANDLER(167)
  DUMMYHANDLER(168)
  DUMMYHANDLER(169)
  DUMMYHANDLER(170)
  DUMMYHANDLER(171)
  DUMMYHANDLER(172)
  DUMMYHANDLER(173)
  DUMMYHANDLER(174)
  DUMMYHANDLER(175)
  DUMMYHANDLER(176)
  DUMMYHANDLER(177)
  DUMMYHANDLER(178)
  DUMMYHANDLER(179)
  DUMMYHANDLER(180)
  DUMMYHANDLER(181)
  DUMMYHANDLER(182)
  DUMMYHANDLER(183)
  DUMMYHANDLER(184)
  DUMMYHANDLER(185)
  DUMMYHANDLER(186)
  DUMMYHANDLER(187)
  DUMMYHANDLER(188)
  DUMMYHANDLER(189)
  DUMMYHANDLER(190)
  DUMMYHANDLER(191)
  DUMMYHANDLER(192)
  DUMMYHANDLER(193)
  DUMMYHANDLER(194)
  DUMMYHANDLER(195)
  DUMMYHANDLER(196)
  DUMMYHANDLER(197)
  DUMMYHANDLER(198)
  DUMMYHANDLER(199)
  DUMMYHANDLER(200)
  DUMMYHANDLER(201)
  DUMMYHANDLER(202)
  DUMMYHANDLER(203)
  DUMMYHANDLER(204)
  DUMMYHANDLER(205)
  DUMMYHANDLER(206)
  DUMMYHANDLER(207)
  DUMMYHANDLER(208)
  DUMMYHANDLER(209)
  DUMMYHANDLER(210)
  DUMMYHANDLER(211)
  DUMMYHANDLER(212)
  DUMMYHANDLER(213)
  DUMMYHANDLER(214)
  DUMMYHANDLER(215)
  DUMMYHANDLER(216)
  DUMMYHANDLER(217)
  DUMMYHANDLER(218)
  DUMMYHANDLER(219)
  DUMMYHANDLER(220)
  DUMMYHANDLER(221)
  DUMMYHANDLER(222)
  DUMMYHANDLER(223)
  DUMMYHANDLER(224)
  DUMMYHANDLER(225)
  DUMMYHANDLER(226)
  DUMMYHANDLER(227)
  DUMMYHANDLER(228)
  DUMMYHANDLER(229)
  DUMMYHANDLER(230)
  DUMMYHANDLER(231)
  DUMMYHANDLER(232)
  DUMMYHANDLER(233)
  DUMMYHANDLER(234)
  DUMMYHANDLER(235)
  DUMMYHANDLER(236)
  DUMMYHANDLER(237)
  DUMMYHANDLER(238)
  DUMMYHANDLER(239)
  DUMMYHANDLER(240)
  DUMMYHANDLER(241)
  DUMMYHANDLER(242)
  DUMMYHANDLER(243)
  DUMMYHANDLER(244)
  DUMMYHANDLER(245)
  DUMMYHANDLER(246)
  DUMMYHANDLER(247)
  DUMMYHANDLER(248)
  DUMMYHANDLER(249)
  DUMMYHANDLER(250)
  DUMMYHANDLER(251)
  DUMMYHANDLER(252)
  DUMMYHANDLER(253)
  DUMMYHANDLER(254)
  DUMMYHANDLER(255)*/
};

extern "C" void arch_irq0_handler()
{
  kprintfd("arch_irq0_handler\n");
  while(1);
}

extern "C" void arch_irq1_handler()
{
  kprintfd("arch_irq1_handler\n");
  while(1);
}

extern "C" void arch_irq2_handler()
{
  while(1)
  kprintfd("arch_irq2_handler\n");
}

extern "C" void arch_irq3_handler()
{
  kprintfd("arch_irq3_handler\n");
  while(1);
}

extern "C" void arch_irq4_handler()
{
  while(1)
    kprintfd("arch_irq4_handler\n");
}

extern "C" void arch_irq5_handler()
{
  while(1)
  kprintfd("arch_irq5_handler\n");
}

extern "C" void arch_irq6_handler()
{
  while(1)
  kprintfd("arch_irq6_handler\n");
}

extern "C" void arch_irq7_handler()
{
  while(1)
    kprintfd("arch_irq7_handler\n");
}
