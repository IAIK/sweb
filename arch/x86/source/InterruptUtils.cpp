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
#include "Terminal.h"
#include "kprintf.h"
#include "Scheduler.h"
#include "debug_bochs.h"

#include "arch_serial.h"
#include "serial.h"
#include "arch_keyboard_manager.h"
#include "arch_bd_manager.h"

#include "Thread.h"
#include "ArchInterrupts.h"

//remove this later
#include "Thread.h"
#include "Loader.h"
#include "Syscall.h"
#include "paging-definitions.h"

  extern "C" void arch_dummyHandler();

// thanks mona
typedef struct {
  uint16 offsetL;  // 0-15bit of offset address
  uint16 selector; // selector address
  uint8 unused;    // unused
  uint8 type;      // type
  uint16 offsetH;  // 16-32bit of offset address
}__attribute__((__packed__)) GateDesc;


void InterruptUtils::initialise()
{
  uint32 i;

  // allocate some memory for our handlers
  GateDesc *interrupt_gates = new GateDesc[NUM_INTERRUPT_HANDLERS];

  for (i=0;i<NUM_INTERRUPT_HANDLERS;++i)
  {
    interrupt_gates[i].offsetL  = ((uint32)(handlers[i].handler)) & 0x0000FFFF;
    interrupt_gates[i].offsetH  = (((uint32)(handlers[i].handler)) & 0xFFFF0000) >> 16;
    //warning FIXME, THIS IS REALLY BAD VOODOO !
    interrupt_gates[i].selector = 8*3;
    // System call use 0x80
    interrupt_gates[i].type     = handlers[i].number == 0x80 ? 0xEE : 0x8E;
    interrupt_gates[i].unused   = 0x00;
  }

  IDTR *idtr = new IDTR();
  idtr->base = (uint32)interrupt_gates;
  idtr->limit = sizeof(GateDesc)*NUM_INTERRUPT_HANDLERS -1;

  lidt(idtr);
}

void InterruptUtils::lidt(IDTR *idtr)
{
  asm volatile("lidt (%0) ": :"q" (idtr));
}

// void InterruptUtils::enableInterrupts(){}
// void InterruptUtils::disableInterrupts(){}

#define DUMMY_HANDLER(x) extern "C" void arch_dummyHandler_##x(); \
  extern "C" void dummyHandler_##x () \
  {\
    kprintfd_nosleep("DUMMY_HANDLER: Spurious INT " #x "\n");\
    kprintf_nosleep("DUMMY_HANDLER: Spurious INT " #x "\n");\
  }

DUMMY_HANDLER(0)
DUMMY_HANDLER(1)
DUMMY_HANDLER(2)
DUMMY_HANDLER(3)
DUMMY_HANDLER(4)
DUMMY_HANDLER(5)
DUMMY_HANDLER(6)
DUMMY_HANDLER(7)
DUMMY_HANDLER(8)
DUMMY_HANDLER(9)
DUMMY_HANDLER(10)
DUMMY_HANDLER(11)
DUMMY_HANDLER(12)
DUMMY_HANDLER(13)
DUMMY_HANDLER(14)
DUMMY_HANDLER(15)
DUMMY_HANDLER(16)
DUMMY_HANDLER(17)
DUMMY_HANDLER(18)
DUMMY_HANDLER(19)
DUMMY_HANDLER(20)
DUMMY_HANDLER(21)
DUMMY_HANDLER(22)
DUMMY_HANDLER(23)
DUMMY_HANDLER(24)
DUMMY_HANDLER(25)
DUMMY_HANDLER(26)
DUMMY_HANDLER(27)
DUMMY_HANDLER(28)
DUMMY_HANDLER(29)
DUMMY_HANDLER(30)
DUMMY_HANDLER(31)
DUMMY_HANDLER(32)
DUMMY_HANDLER(33)
DUMMY_HANDLER(34)
DUMMY_HANDLER(35)
DUMMY_HANDLER(36)
DUMMY_HANDLER(37)
DUMMY_HANDLER(38)
DUMMY_HANDLER(39)
DUMMY_HANDLER(40)
DUMMY_HANDLER(41)
DUMMY_HANDLER(42)
DUMMY_HANDLER(43)
DUMMY_HANDLER(44)
DUMMY_HANDLER(45)
DUMMY_HANDLER(46)
DUMMY_HANDLER(47)
DUMMY_HANDLER(48)
DUMMY_HANDLER(49)
DUMMY_HANDLER(50)
DUMMY_HANDLER(51)
DUMMY_HANDLER(52)
DUMMY_HANDLER(53)
DUMMY_HANDLER(54)
DUMMY_HANDLER(55)
DUMMY_HANDLER(56)
DUMMY_HANDLER(57)
DUMMY_HANDLER(58)
DUMMY_HANDLER(59)
DUMMY_HANDLER(60)
DUMMY_HANDLER(61)
DUMMY_HANDLER(62)
DUMMY_HANDLER(63)
DUMMY_HANDLER(64)
DUMMY_HANDLER(65)
DUMMY_HANDLER(66)
DUMMY_HANDLER(67)
DUMMY_HANDLER(68)
DUMMY_HANDLER(69)
DUMMY_HANDLER(70)
DUMMY_HANDLER(71)
DUMMY_HANDLER(72)
DUMMY_HANDLER(73)
DUMMY_HANDLER(74)
DUMMY_HANDLER(75)
DUMMY_HANDLER(76)
DUMMY_HANDLER(77)
DUMMY_HANDLER(78)
DUMMY_HANDLER(79)
DUMMY_HANDLER(80)
DUMMY_HANDLER(81)
DUMMY_HANDLER(82)
DUMMY_HANDLER(83)
DUMMY_HANDLER(84)
DUMMY_HANDLER(85)
DUMMY_HANDLER(86)
DUMMY_HANDLER(87)
DUMMY_HANDLER(88)
DUMMY_HANDLER(89)
DUMMY_HANDLER(90)
DUMMY_HANDLER(91)
DUMMY_HANDLER(92)
DUMMY_HANDLER(93)
DUMMY_HANDLER(94)
DUMMY_HANDLER(95)
DUMMY_HANDLER(96)
DUMMY_HANDLER(97)
DUMMY_HANDLER(98)
DUMMY_HANDLER(99)
DUMMY_HANDLER(100)
DUMMY_HANDLER(101)
DUMMY_HANDLER(102)
DUMMY_HANDLER(103)
DUMMY_HANDLER(104)
DUMMY_HANDLER(105)
DUMMY_HANDLER(106)
DUMMY_HANDLER(107)
DUMMY_HANDLER(108)
DUMMY_HANDLER(109)
DUMMY_HANDLER(110)
DUMMY_HANDLER(111)
DUMMY_HANDLER(112)
DUMMY_HANDLER(113)
DUMMY_HANDLER(114)
DUMMY_HANDLER(115)
DUMMY_HANDLER(116)
DUMMY_HANDLER(117)
DUMMY_HANDLER(118)
DUMMY_HANDLER(119)
DUMMY_HANDLER(120)
DUMMY_HANDLER(121)
DUMMY_HANDLER(122)
DUMMY_HANDLER(123)
DUMMY_HANDLER(124)
DUMMY_HANDLER(125)
DUMMY_HANDLER(126)
DUMMY_HANDLER(127)
//DUMMY_HANDLER(128)
DUMMY_HANDLER(129)
DUMMY_HANDLER(130)
DUMMY_HANDLER(131)
DUMMY_HANDLER(132)
DUMMY_HANDLER(133)
DUMMY_HANDLER(134)
DUMMY_HANDLER(135)
DUMMY_HANDLER(136)
DUMMY_HANDLER(137)
DUMMY_HANDLER(138)
DUMMY_HANDLER(139)
DUMMY_HANDLER(140)
DUMMY_HANDLER(141)
DUMMY_HANDLER(142)
DUMMY_HANDLER(143)
DUMMY_HANDLER(144)
DUMMY_HANDLER(145)
DUMMY_HANDLER(146)
DUMMY_HANDLER(147)
DUMMY_HANDLER(148)
DUMMY_HANDLER(149)
DUMMY_HANDLER(150)
DUMMY_HANDLER(151)
DUMMY_HANDLER(152)
DUMMY_HANDLER(153)
DUMMY_HANDLER(154)
DUMMY_HANDLER(155)
DUMMY_HANDLER(156)
DUMMY_HANDLER(157)
DUMMY_HANDLER(158)
DUMMY_HANDLER(159)
DUMMY_HANDLER(160)
DUMMY_HANDLER(161)
DUMMY_HANDLER(162)
DUMMY_HANDLER(163)
DUMMY_HANDLER(164)
DUMMY_HANDLER(165)
DUMMY_HANDLER(166)
DUMMY_HANDLER(167)
DUMMY_HANDLER(168)
DUMMY_HANDLER(169)
DUMMY_HANDLER(170)
DUMMY_HANDLER(171)
DUMMY_HANDLER(172)
DUMMY_HANDLER(173)
DUMMY_HANDLER(174)
DUMMY_HANDLER(175)
DUMMY_HANDLER(176)
DUMMY_HANDLER(177)
DUMMY_HANDLER(178)
DUMMY_HANDLER(179)
DUMMY_HANDLER(180)
DUMMY_HANDLER(181)
DUMMY_HANDLER(182)
DUMMY_HANDLER(183)
DUMMY_HANDLER(184)
DUMMY_HANDLER(185)
DUMMY_HANDLER(186)
DUMMY_HANDLER(187)
DUMMY_HANDLER(188)
DUMMY_HANDLER(189)
DUMMY_HANDLER(190)
DUMMY_HANDLER(191)
DUMMY_HANDLER(192)
DUMMY_HANDLER(193)
DUMMY_HANDLER(194)
DUMMY_HANDLER(195)
DUMMY_HANDLER(196)
DUMMY_HANDLER(197)
DUMMY_HANDLER(198)
DUMMY_HANDLER(199)
DUMMY_HANDLER(200)
DUMMY_HANDLER(201)
DUMMY_HANDLER(202)
DUMMY_HANDLER(203)
DUMMY_HANDLER(204)
DUMMY_HANDLER(205)
DUMMY_HANDLER(206)
DUMMY_HANDLER(207)
DUMMY_HANDLER(208)
DUMMY_HANDLER(209)
DUMMY_HANDLER(210)
DUMMY_HANDLER(211)
DUMMY_HANDLER(212)
DUMMY_HANDLER(213)
DUMMY_HANDLER(214)
DUMMY_HANDLER(215)
DUMMY_HANDLER(216)
DUMMY_HANDLER(217)
DUMMY_HANDLER(218)
DUMMY_HANDLER(219)
DUMMY_HANDLER(220)
DUMMY_HANDLER(221)
DUMMY_HANDLER(222)
DUMMY_HANDLER(223)
DUMMY_HANDLER(224)
DUMMY_HANDLER(225)
DUMMY_HANDLER(226)
DUMMY_HANDLER(227)
DUMMY_HANDLER(228)
DUMMY_HANDLER(229)
DUMMY_HANDLER(230)
DUMMY_HANDLER(231)
DUMMY_HANDLER(232)
DUMMY_HANDLER(233)
DUMMY_HANDLER(234)
DUMMY_HANDLER(235)
DUMMY_HANDLER(236)
DUMMY_HANDLER(237)
DUMMY_HANDLER(238)
DUMMY_HANDLER(239)
DUMMY_HANDLER(240)
DUMMY_HANDLER(241)
DUMMY_HANDLER(242)
DUMMY_HANDLER(243)
DUMMY_HANDLER(244)
DUMMY_HANDLER(245)
DUMMY_HANDLER(246)
DUMMY_HANDLER(247)
DUMMY_HANDLER(248)
DUMMY_HANDLER(249)
DUMMY_HANDLER(250)
DUMMY_HANDLER(251)
DUMMY_HANDLER(252)
DUMMY_HANDLER(253)
DUMMY_HANDLER(254)
DUMMY_HANDLER(255)

typedef struct ArchThreadInfo
{
  uint32  eip;       // 0
  uint32  cs;        // 4
  uint32  eflags;    // 8
  uint32  eax;       // 12
  uint32  ecx;       // 16
  uint32  edx;       // 20
  uint32  ebx;       // 24
  uint32  esp;       // 28
  uint32  ebp;       // 32
  uint32  esi;       // 36
  uint32  edi;       // 40
  uint32  ds;        // 44
  uint32  es;        // 48
  uint32  fs;        // 52
  uint32  gs;        // 56
  uint32  ss;        // 60
  uint32  dpl;       // 64
  uint32  esp0;      // 68  call neo_%1
  uint32  ss0;       // 72
  uint32  cr3;       // 76
  uint32  fpu[27];   // 80
};

extern ArchThreadInfo *currentThreadInfo;
extern Thread *currentThread;

#define IRQ_HANDLER(x) extern "C" void arch_irqHandler_##x(); \
  extern "C" void irqHandler_##x ()  {  \
    kprintfd_nosleep("IRQ_HANDLER: Spurious IRQ " #x "\n"); \
    kprintf_nosleep("IRQ_HANDLER: Spurious IRQ " #x "\n"); \
    ArchInterrupts::EndOfInterrupt(x); \
  }; \

extern "C" void arch_irqHandler_0();
extern "C" void arch_switchThreadKernelToKernel();
extern "C" void arch_switchThreadKernelToKernelPageDirChange();
extern "C" void arch_switchThreadToUserPageDirChange();
extern "C" void irqHandler_0()
{
  static uint32 heart_beat_value = 0;
  // static uint32 leds = 0;
  // static uint32 ctr = 0;
  char* fb = (char*)0xC00B8000;
  switch (heart_beat_value)
  {
    default:
    case 0:
    fb[0] = '/';
    fb[1] = 0x9f;
    break;
    case 1:
    fb[0] = '-';
    fb[1] = 0x9f;
    break;
    case 2:
    fb[0] = '\\';
    fb[1] = 0x9f;
    break;
    case 3:
    fb[0] = '|';
    fb[1] = 0x9f;
    break;
  }
  heart_beat_value = (heart_beat_value + 1) % 4;

  // outportb( 0xED, 0x60 );  // "set LEDs" command
  // outportb( leds, 0x60 );
  // if (ctr == 9)
  // leds = (leds + 1) % (1 << 3);
  // ctr = (ctr + 1) % 10;

  // kprintfd_nosleep("irq0: Tick\n");
  // writeLine2Bochs((uint8 const *)"Enter irq Handler 0\n");

  uint32 ret = Scheduler::instance()->schedule();
  switch (ret)
  {
    case 0:
      // kprintfd_nosleep("irq0: Going to leave irq Handler 0 to kernel\n");
      ArchInterrupts::EndOfInterrupt(0);
      arch_switchThreadKernelToKernelPageDirChange();
    case 1:
      // kprintfd_nosleep("irq0: Going to leave irq Handler 0 to user\n");
      ArchInterrupts::EndOfInterrupt(0);
      arch_switchThreadToUserPageDirChange();
    default:
      kprintfd_nosleep("irq0: Panic in int 0 handler\n");
      for(;;);
  }
}

extern "C" void arch_irqHandler_65();
extern "C" void irqHandler_65()
{
  uint32 ret = Scheduler::instance()->schedule();
  switch (ret)
  {
    case 0:
      // kprintfd_nosleep("irq65: Going to leave int Handler 65 to kernel\n");
      arch_switchThreadKernelToKernelPageDirChange();
    case 1:
      // kprintfd_nosleep("irq65: Going to leave int Handler 65 to user\n");
      arch_switchThreadToUserPageDirChange();

    default:
      kprintfd_nosleep("irq65: Panic in int 65 handler\n");
      for(;;);
  }
}


extern "C" void arch_interruptHandler_0();
extern "C" void arch_interruptHandler_0()
{
  kpanict((uint8 *) "DIVISION ERROR\n");
}

extern "C" void arch_pageFaultHandler();
extern "C" void pageFaultHandler(uint32 address, uint32 error)
{
  //--------Start "just for Debugging"-----------  
  uint32 const __attribute__((unused)) flag_p = 0x1 << 0; // =0: pf caused because pt was not present; =1: protection violation
  uint32 const __attribute__((unused)) flag_rw = 0x1 << 1; // pf caused by a 1=write/0=read
  uint32 const __attribute__((unused)) flag_us = 0x1 << 2; // pf caused in 1=usermode/0=supervisormode
  uint32 const __attribute__((unused)) flag_rsvd = 0x1 << 3; // pf caused by reserved bits
    
  // uint32 cr2=0xffff;
  // __asm__("movl %%cr2, %0"
  // :"=a"(cr2)
  // :);
  
  debug(A_INTERRUPTS | PM, "pageFaultHandler( address: %x, error: page_present=%d writing=%d user=%d rsvd=%d)\n\t(currentThread: %x %s, switch_to_userspace_:%d)\n",
      address,
      error&flag_p,
      (error&flag_rw) >> 1,
      (error&flag_us) >> 2,
      (error&flag_rsvd) >> 3,
      currentThread,currentThread->getName(),
      currentThread->switch_to_userspace_);
  if (error)
  {
    if (error&flag_p)
    {
      debug(A_INTERRUPTS | PM, "Ouch, We got a pagefault even though the page mapping is present\n");
      debug(A_INTERRUPTS | PM, "Apparently ");
      if (error&flag_us)
        debug(A_INTERRUPTS | PM, "a userprogram ");
      else
        debug(A_INTERRUPTS | PM, "some kernel code ");
      debug(A_INTERRUPTS | PM, " tried to ");
      if (error&flag_rw)
        debug(A_INTERRUPTS | PM, "write to ");
      else
        debug(A_INTERRUPTS | PM, "read from ");
      debug(A_INTERRUPTS | PM, "address 0x%x\n",address);
      if (address >= 2U*1024U*1024U*1024U)
      debug(A_INTERRUPTS | PM, "Likely the PageTable Flags forbid this operation\n");
        
      page_directory_entry *page_directory = (page_directory_entry *) ArchMemory::get3GBAdressOfPPN(currentThread->loader_->page_dir_page_);
      uint32 virtual_page = address / PAGE_SIZE;
      uint32 pde_vpn = virtual_page / PAGE_TABLE_ENTRIES;
      uint32 pte_vpn = virtual_page % PAGE_TABLE_ENTRIES;
      if (page_directory[pde_vpn].pde4k.present)
      {
        if (page_directory[pde_vpn].pde4m.use_4_m_pages)
        {
          debug(A_INTERRUPTS | PM, "Page %d is a 4MiB Page\n",virtual_page);
          debug(A_INTERRUPTS | PM, "Page %d Flags are: writeable:%d, userspace_accessible:%d,\n", virtual_page, page_directory[pde_vpn].pde4m.writeable, page_directory[pde_vpn].pde4m.user_access);
        }
        else
        {
          page_table_entry *pte_base = (page_table_entry *) ArchMemory::get3GBAdressOfPPN(page_directory[pde_vpn].pde4k.page_table_base_address);
          debug(A_INTERRUPTS | PM, "Page %d is a 4KiB Page\n",virtual_page);
          debug(A_INTERRUPTS | PM, "Page %d Flags are: present:%d, writeable:%d, userspace_accessible:%d,\n", virtual_page, pte_base[pte_vpn].present, pte_base[pte_vpn].writeable, pte_base[pte_vpn].user_access);
        }
      }
      else
        debug(A_INTERRUPTS | PM, "WTF ? PDE non-present but Exception present flag was set ?\n");
    }
    else
    {
      if (address >= 2U*1024U*1024U*1024U)
      {
        debug(A_INTERRUPTS | PM, "The virtual page we accessed was not mapped to a physical page\n");
        if (error&flag_us)
        {
          debug(A_INTERRUPTS | PM, "WARNING: Your Userspace Programm tried to read from an unmapped address >2GiB\n");
          debug(A_INTERRUPTS | PM, "WARNING: most likey there is an pointer error somewhere\n");
        }
        else
        {
          // remove this error check if your implementation swaps out kernel pages
          debug(A_INTERRUPTS | PM, "WARNING: This is unusual for addresses above 2Gb, unless you are swapping kernel pages\n");
          debug(A_INTERRUPTS | PM, "WARNING: most likey there is an pointer error somewhere\n");
        }            
      }
      else
      {
        //debug(A_INTERRUPTS | PM, "The virtual page we accessed was not mapped to a physical page\n");
        //debug(A_INTERRUPTS | PM, "this is normal and the Loader will propably take care of it now\n");
      }
    }      
  }

  //ArchThreads::printThreadRegisters(currentThread,0);
  //ArchThreads::printThreadRegisters(currentThread,1);

  // kprintfd_nosleep( "CR3 =  %X, pg_num = %X, pg3GB = %x \n\n",
	  // currentThread->user_arch_thread_info_->cr3,
	  // currentThread->loader_->page_dir_page_,
	  // ArchMemory::get3GBAdressOfPPN(currentThread->loader_->page_dir_page_) );

  // page_directory_entry *cpd = (page_directory_entry *) ArchMemory::get3GBAdressOfPPN(currentThread->loader_->page_dir_page_);
  // uint32 i = 0;
  // uint32 j = 0;
  // for( i = 0; i < 512; i++ )
  // {
    // if( cpd[ i ].pde4k.present )
    // {
      // kprintfd_nosleep( " i %d, present %d, where %Xm where 3G : %X \n",
		  // i,
		  // cpd[ i ].pde4k.present,
		  // cpd[ i ].pde4k.page_table_base_address,
		  // ArchMemory::get3GBAdressOfPPN( cpd[ i ].pde4k.page_table_base_address )
		  // );
    // }
    // if( cpd[ i ].pde4k.present )
    // {
      // page_table_entry *cpt = (page_table_entry *) ArchMemory::get3GBAdressOfPPN( cpd[ i ].pde4k.page_table_base_address );
      // for( j = 0; j < 256; j++ )
      // {
        // if( cpt[ j ].present )
        // {
          // kprintfd_nosleep( "\t j %d, present %d, where %X \n",
		// j,
		// cpt[ j ].present,
		// cpt[ j ].page_base_address );
	// }
      // }
    // }
  // }

  //--------End "just for Debugging"-----------
    
  //kprintfd_nosleep("PageFault:: switching to Kernelspace (currentThread=%x %s)\n",currentThread,currentThread->getName());
  currentThread->switch_to_userspace_ = false;
  currentThreadInfo = currentThread->kernel_arch_thread_info_;
  ArchInterrupts::enableInterrupts();

  //lets hope this Exeption wasn't thrown during a TaskSwitch
  if (! (error & flag_p) && address < 2U*1024U*1024U*1024U && currentThread->loader_)
  {
    currentThread->loader_->loadOnePageSafeButSlow(address); //load stuff
    // ArchInterrupts::enableInterrupts(); //previous EFLAGS get restored anyway, so this is not necessary
  }
  else
  {
    if (currentThread->loader_)
      Syscall::exit(9999);
    else
      currentThread->kill();
  }
  //kprintfd_nosleep("PageFault: returning to Userspace (currentThread=%x %s)\n",currentThread,currentThread->getName());
  ArchInterrupts::disableInterrupts();
  currentThread->switch_to_userspace_ = true;
  currentThreadInfo = currentThread->user_arch_thread_info_;
  //kprintfd_nosleep("PageFault: done (currentThread=%x %s)\n",currentThread,currentThread->getName());
  arch_switchThreadToUserPageDirChange();
}

extern "C" void arch_irqHandler_1();
extern "C" void irqHandler_1()
{
  KeyboardManager::getInstance()->serviceIRQ( );
  ArchInterrupts::EndOfInterrupt(1);
}

extern "C" void arch_irqHandler_3();
extern "C" void irqHandler_3()
{
  kprintfd_nosleep( "IRQ 3 called\n" );
  SerialManager::getInstance()->service_irq( 3 );
  ArchInterrupts::EndOfInterrupt(3);
  kprintfd_nosleep( "IRQ 3 ended\n" );
}

extern "C" void arch_irqHandler_4();
extern "C" void irqHandler_4()
{
  kprintfd_nosleep( "IRQ 4 called\n" );
  SerialManager::getInstance()->service_irq( 4 );
  ArchInterrupts::EndOfInterrupt(4);
  kprintfd_nosleep( "IRQ 4 ended\n" );
}

extern "C" void arch_irqHandler_6();
extern "C" void irqHandler_6()
{
  kprintfd_nosleep( "IRQ 6 called\n" );
  kprintfd_nosleep( "IRQ 6 ended\n" );
}

extern "C" void arch_irqHandler_9();
extern "C" void irqHandler_9()
{
  kprintfd_nosleep( "IRQ 9 called\n" );
  BDManager::getInstance()->serviceIRQ( 9 );
  ArchInterrupts::EndOfInterrupt(9);
}

extern "C" void arch_irqHandler_11();
extern "C" void irqHandler_11()
{
  kprintfd_nosleep( "IRQ 11 called\n" );
  BDManager::getInstance()->serviceIRQ( 11 );
  ArchInterrupts::EndOfInterrupt(11);
}

extern "C" void arch_irqHandler_14();
extern "C" void irqHandler_14()
{
  kprintfd_nosleep( "IRQ 14 called\n" );
  BDManager::getInstance()->serviceIRQ( 14 );
  ArchInterrupts::EndOfInterrupt(14);
}

extern "C" void arch_irqHandler_15();
extern "C" void irqHandler_15()
{
  kprintfd_nosleep( "IRQ 15 called\n" );
  BDManager::getInstance()->serviceIRQ( 15 );
  ArchInterrupts::EndOfInterrupt(15);
}

extern "C" void arch_syscallHandler();
extern "C" void syscallHandler()
{

  //kprintfd_nosleep("syscallHANDLER called, interrupts are %d (currentThread=%x %s)\n",ArchInterrupts::testIFSet(),currentThread,currentThread->getName());
  //ArchThreads::printThreadRegisters(currentThread,0);
  //ArchThreads::printThreadRegisters(currentThread,1);
   // ok, find out the current thread
  //currentThreadInfo = currentThread->kernel_arch_thread_info_;
  //~ kprintfd_nosleep("syscallHANDLER: thread: eax: %x; ebx: %x; ecx: %x; edx: %x;\n",currentThread->user_arch_thread_info_->eax,
                  //~ currentThread->user_arch_thread_info_->ebx,
                  //~ currentThread->user_arch_thread_info_->ecx,
                  //~ currentThread->user_arch_thread_info_->edx);

  // a int 0x80 instruction takes two bytes in x86 asm
  // to make sure we skip this one after syscall exit
  // we have to increment the eip
  // add on, ever since the very first pmode machine
  // this is not needed anymore as the machine is smart
  // enough to do this on a trap

  //kprintfd_nosleep("syscallHANDLER: switching to Kernelspace (currentThread=%x %s)\n",currentThread,currentThread->getName());

  currentThread->switch_to_userspace_ = false;
  currentThreadInfo = currentThread->kernel_arch_thread_info_;
  ArchInterrupts::enableInterrupts();

  currentThread->user_arch_thread_info_->eax =
    Syscall::syscallException(currentThread->user_arch_thread_info_->eax,
                  currentThread->user_arch_thread_info_->ebx,
                  currentThread->user_arch_thread_info_->ecx,
                  currentThread->user_arch_thread_info_->edx,
                  currentThread->user_arch_thread_info_->esi,
                  currentThread->user_arch_thread_info_->edi);

  //kprintfd_nosleep("syscallHANDLER: returning to Userspace (currentThread=%x %s)\n",currentThread,currentThread->getName());
  ArchInterrupts::disableInterrupts();
  currentThread->switch_to_userspace_ = true;
  currentThreadInfo =  currentThread->user_arch_thread_info_;
  //ArchThreads::printThreadRegisters(currentThread,1);
  //kprintfd_nosleep("syscallHANDLER: done (currentThread=%x %s)\n",currentThread,currentThread->getName());
  arch_switchThreadToUserPageDirChange();
}

//IRQ_HANDLER(1)
IRQ_HANDLER(2)
//IRQ_HANDLER(3)
//IRQ_HANDLER(4)
IRQ_HANDLER(5)
//IRQ_HANDLER(6)
IRQ_HANDLER(7)
IRQ_HANDLER(8)
//IRQ_HANDLER(9)
IRQ_HANDLER(10)
//IRQ_HANDLER(11)
IRQ_HANDLER(12)
IRQ_HANDLER(13)
//IRQ_HANDLER(14)
//IRQ_HANDLER(15)

extern "C" void arch_dummyHandler();

#define DUMMYHANDLER(X) {X, &arch_dummyHandler_##X},
#define IRQHANDLER(X) {X + 32, &arch_irqHandler_##X},
InterruptHandlers InterruptUtils::handlers[NUM_INTERRUPT_HANDLERS] = {
  {0, &arch_interruptHandler_0},
  DUMMYHANDLER(1)
  DUMMYHANDLER(2)
  DUMMYHANDLER(3)
  DUMMYHANDLER(4)
  DUMMYHANDLER(5)
  DUMMYHANDLER(6)
  DUMMYHANDLER(7)
  DUMMYHANDLER(8)
  DUMMYHANDLER(9)
  DUMMYHANDLER(10)
  DUMMYHANDLER(11)
  DUMMYHANDLER(12)
  DUMMYHANDLER(13)
  {14, &arch_pageFaultHandler},
  DUMMYHANDLER(15)
  DUMMYHANDLER(16)
  DUMMYHANDLER(17)
  DUMMYHANDLER(18)
  DUMMYHANDLER(19)
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
  DUMMYHANDLER(255)
};
