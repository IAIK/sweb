/**
 * $Id: main.cpp,v 1.94 2005/09/20 19:07:41 btittelbach Exp $
 *
 * $Log: main.cpp,v $
 * Revision 1.93  2005/09/20 08:05:08  btittelbach
 * +kprintf flush fix: even though it worked fine before, now it works fine in theory as well ;->
 * +Condition cleanup
 * +FiFoDRBOSS now obsolete and removed
 * +added disk.img that nelle forgot to check in
 *
 * Revision 1.92  2005/09/18 20:25:05  nelles
 *
 *
 * Block devices update.
 * See BDRequest and BDManager on how to use this.
 * Currently ATADriver is functional. The driver tries to detect if IRQ
 * mode is available and adjusts the mode of operation. Currently PIO
 * modes with IRQ or without it are supported.
 *
 * TODO:
 * - add block PIO mode to read or write multiple sectors within one IRQ
 * - add DMA and UDMA mode :)
 *
 *
 *  Committing in .
 *
 *  Modified Files:
 *  	arch/common/include/ArchInterrupts.h
 *  	arch/x86/source/ArchInterrupts.cpp
 *  	arch/x86/source/InterruptUtils.cpp
 *  	common/include/kernel/TestingThreads.h
 *  	common/source/kernel/Makefile
 *  	common/source/kernel/Scheduler.cpp
 *  	common/source/kernel/main.cpp utils/bochs/bochsrc
 *  Added Files:
 *  	arch/x86/include/arch_bd_ata_driver.h
 *  	arch/x86/include/arch_bd_driver.h
 *  	arch/x86/include/arch_bd_ide_driver.h
 *  	arch/x86/include/arch_bd_io.h
 * 	arch/x86/include/arch_bd_manager.h
 *  	arch/x86/include/arch_bd_request.h
 *  	arch/x86/include/arch_bd_virtual_device.h
 *  	arch/x86/source/arch_bd_ata_driver.cpp
 *  	arch/x86/source/arch_bd_ide_driver.cpp
 *  	arch/x86/source/arch_bd_manager.cpp
 * 	arch/x86/source/arch_bd_virtual_device.cpp
 *
 * Revision 1.91  2005/09/17 16:21:57  nelles
 *
 *  Small bugfix in keyboard manager.
 *  Shift, Caps, Num etc. status now initialized by ctor
 *
 *  Committing in .
 *
 *  Modified Files:
 *  	arch/x86/source/arch_keyboard_manager.cpp
 *
 * Revision 1.90  2005/09/16 15:47:41  btittelbach
 * +even more KeyboardInput Bugfixes
 * +intruducing: kprint_buffer(..) (console write should never be used directly from anything with IF=0)
 * +Thread now remembers its Terminal
 * +Syscalls are USEABLE !! :-) IF=1 !!
 * +Syscalls can block now ! ;-) Waiting for Input...
 * +more other Bugfixes
 *
 * Revision 1.89  2005/09/16 12:47:41  btittelbach
 * Second PatchThursday:
 * +KeyboardInput SyncStructure Rewrite
 * +added RingBuffer
 * +debugged FiFoDRBOSS (even though now obsolete)
 * +improved FiFo
 * +more debugging
 * Added Files:
 *  	common/include/ipc/RingBuffer.h
 *
 * Revision 1.88  2005/09/16 00:54:13  btittelbach
 * Small not-so-good Sync-Fix that works before Total-Syncstructure-Rewrite
 *
 * Revision 1.87  2005/09/15 18:47:07  btittelbach
 * FiFoDRBOSS should only be used in interruptHandler Kontext, for everything else use FiFo
 * IdleThread now uses hlt instead of yield.
 *
 * Revision 1.86  2005/09/15 17:51:13  nelles
 *
 *
 *  Massive update. Like PatchThursday.
 *  Keyboard is now available.
 *  Each Terminal has a buffer attached to it and threads should read the buffer
 *  of the attached terminal. See TestingThreads.h in common/include/kernel for
 *  example of how to do it.
 *  Switching of the terminals is done with the SHFT+F-keys. (CTRL+Fkeys gets
 *  eaten by X on my machine and does not reach Bochs).
 *  Lot of smaller modifications, to FiFo, Mutex etc.
 *
 *  Committing in .
 *
 *  Modified Files:
 *  	arch/x86/source/InterruptUtils.cpp
 *  	common/include/console/Console.h
 *  	common/include/console/Terminal.h
 *  	common/include/console/TextConsole.h common/include/ipc/FiFo.h
 *  	common/include/ipc/FiFoDRBOSS.h common/include/kernel/Mutex.h
 *  	common/source/console/Console.cpp
 *  	common/source/console/Makefile
 *  	comm
 *
 *
 *  Massive update. Like PatchThursday.
 *  Keyboard is now available.
 *  Each Terminal has a buffer attached to it and threads should read the buffer
 *  of the attached terminal. See TestingThreads.h in common/include/kernel for
 *  example of how to do it.
 *  Switching of the terminals is done with the SHFT+F-keys. (CTRL+Fkeys gets
 *  eaten by X on my machine and does not reach Bochs).
 *  Lot of smaller modifications, to FiFo, Mutex etc.
 *
 *  Committing in .
 *
 *  Modified Files:
 *  	arch/x86/source/InterruptUtils.cpp
 *  	common/include/console/Console.h
 *  	common/include/console/Terminal.h
 *  	common/include/console/TextConsole.h common/include/ipc/FiFo.h
 *  	common/include/ipc/FiFoDRBOSS.h common/include/kernel/Mutex.h
 *  	common/source/console/Console.cpp
 *  	common/source/console/Makefile
 *  	common/source/console/Terminal.cpp
 *  	common/source/console/TextConsole.cpp
 *  	common/source/kernel/Condition.cpp
 *  	common/source/kernel/Mutex.cpp
 *  	common/source/kernel/Scheduler.cpp
 *  	common/source/kernel/Thread.cpp common/source/kernel/main.cpp
 *  Added Files:
 *  	arch/x86/include/arch_keyboard_manager.h
 *  	arch/x86/source/arch_keyboard_manager.cpp
 *
 * Revision 1.85  2005/09/13 22:15:52  btittelbach
 * small BugFix: Scheduler really works now
 *
 * Revision 1.84  2005/09/13 15:00:51  btittelbach
 * Prepare to be Synchronised...
 * kprintf_nosleep works now
 * scheduler/list still needs to be fixed
 *
 * Revision 1.83  2005/09/12 14:22:25  btittelbach
 * tried cleaning up Scheduler by using List-Rotate instead of MemoryAllocation
 * but then found out, that this could NEVER reliably work with the kind of
 * List we use :-(
 *
 * Revision 1.82  2005/09/10 19:25:27  qiangchen
 *  21:24:09 up 14:16,  3 users,  load average: 0.08, 0.09, 0.14
 * USER     TTY      FROM              LOGIN@   IDLE   JCPU   PCPU WHAT
 * chen     :0       -                12:11   ?xdm?   1:01m  1.35s /usr/bin/gnome-
 * chen     pts/0    :0.0             12:15    1.00s  0.34s  0.03s cvs commit
 * chen     pts/1    :0.0             12:33    5:23m  3.13s  0.04s -bash
 *
 * Revision 1.81  2005/09/07 00:33:52  btittelbach
 * +More Bugfixes
 * +Character Queue (FiFoDRBOSS) from irq with Synchronisation that actually works
 *
 * Revision 1.80  2005/09/06 09:56:50  btittelbach
 * +Thread Names
 * +stdin Test Example
 *
 * Revision 1.79  2005/09/05 23:01:24  btittelbach
 * Keyboard Input Handler
 * + several Bugfixes
 *
 * Revision 1.78  2005/09/03 21:54:45  btittelbach
 * Syscall Testprogramm, actually works now ;-) ;-)
 * Test get autocompiled and autoincluded into kernel
 * one kprintfd bug fixed
 *
 * Revision 1.77  2005/09/03 18:20:14  nomenquis
 * pseudo fs works now
 *
 * Revision 1.76  2005/09/03 17:08:34  nomenquis
 * added support for grub modules
 *
 * Revision 1.75  2005/09/02 17:57:58  davrieb
 * preparations to  build a standalone filesystem testsuite
 *
 * Revision 1.74  2005/08/26 13:58:24  nomenquis
 * finally even the syscall handler does that it is supposed to do
 *
 * Revision 1.73  2005/08/26 12:01:25  nomenquis
 * pagefaults in userspace now should really really really work
 *
 * Revision 1.72  2005/08/19 21:14:15  btittelbach
 * Debugging the Debugging Code
 *
 * Revision 1.71  2005/08/11 16:46:57  davrieb
 * add PathWalker
 *
 * Revision 1.70  2005/08/11 16:18:02  nomenquis
 * fixed a bug
 *
 * Revision 1.69  2005/08/07 16:47:25  btittelbach
 * More nice synchronisation Experiments..
 * RaceCondition/kprintf_nosleep related ?/infinite memory write loop Error still not found
 * kprintfd doesn't use a buffer anymore, as output_bochs blocks anyhow, should propably use some arch-specific interface instead
 *
 * Revision 1.68  2005/08/04 20:47:43  btittelbach
 * Where is the Bug, maybe I will see something tomorrow that I didn't see today
 *
 * Revision 1.67  2005/08/04 17:49:22  btittelbach
 * Improved (documented) arch_PageFaultHandler
 * Solution to Userspace Bug still missing....
 *
 * Revision 1.66  2005/08/03 11:56:57  btittelbach
 * Evil PageFaultBug now gets bigger... (but hopefully better to debug)
 *
 * Revision 1.65  2005/08/02 19:47:54  btittelbach
 * Syscalls: there is some very evil bug still hidden here, what did I forget ?
 *
 * Revision 1.64  2005/08/01 08:41:15  nightcreature
 * removed include of mutliboot.h as it conflicts with xen and isn't really needed
 *
 * Revision 1.63  2005/07/27 13:43:48  btittelbach
 * Interrupt On/Off Autodetection in Kprintf
 *
 * Revision 1.62  2005/07/27 10:04:26  btittelbach
 * kprintf_nosleep and kprintfd_nosleep now works
 * Output happens in dedicated Thread using VERY EVIL Mutex Hack
 *
 * Revision 1.61  2005/07/24 17:02:59  nomenquis
 * lots of changes for new console stuff
 *
 * Revision 1.60  2005/07/21 19:33:41  btittelbach
 * Fifo blocks now, and students still have the opportunity to implement a real cv
 *
 * Revision 1.59  2005/07/21 19:08:41  btittelbach
 * Jö schön, Threads u. Userprozesse werden ordnungsgemäß beendet
 * Threads können schlafen, Mutex benutzt das jetzt auch
 * Jetzt muß nur der Mutex auch überall verwendet werden
 *
 * Revision 1.58  2005/07/21 11:50:06  btittelbach
 * Basic Syscall
 *
 * Revision 1.57  2005/07/12 21:05:39  btittelbach
 * Lustiges Spielen mit UserProgramm Terminierung
 *
 * Revision 1.56  2005/07/12 19:52:25  btittelbach
 * Debugged evil evil double-linked-list bug
 *
 * Revision 1.55  2005/07/12 17:53:13  btittelbach
 * Bochs SerialConsole ist jetzt lesbar
 *
 * Revision 1.54  2005/07/07 15:00:48  davrieb
 * fix the test
 *
 * Revision 1.53  2005/07/07 14:06:55  davrieb
 * more fs testing
 *
 * Revision 1.52  2005/07/07 13:20:10  lythien
 * only for david
 *
 * Revision 1.50  2005/07/06 13:29:37  btittelbach
 * testing
 *
 * Revision 1.49  2005/07/05 20:22:56  btittelbach
 * some changes
 *
 * Revision 1.48  2005/07/05 17:29:48  btittelbach
 * new kprintf(d) Policy:
 * [Class::]Function: before start of debug message
 * Function can be abbreviated "ctor" if Constructor
 * use kprintfd where possible
 *
 * Revision 1.47  2005/06/14 18:22:37  btittelbach
 * RaceCondition anfälliges LoadOnDemand implementiert,
 * sollte optimalerweise nicht im InterruptKontext laufen
 *
 * Revision 1.46  2005/06/14 13:54:55  nomenquis
 * foobarpratz
 *
 * Revision 1.45  2005/06/05 07:59:35  nelles
 * The kprintf_debug or kprintfd are finished
 *
 * Revision 1.44  2005/06/04 19:41:26  nelles
 *
 * Serial ports now fully fuctional and tested ....
 *
 * Revision 1.43  2005/05/31 18:13:14  nomenquis
 * fixed compile errors
 *
 * Revision 1.42  2005/05/31 17:29:16  nomenquis
 * userspace
 *
 * Revision 1.39  2005/05/25 08:27:49  nomenquis
 * cr3 remapping finally really works now
 *
 * Revision 1.38  2005/05/19 15:43:43  btittelbach
 * Ans�ze fr eine UserSpace Verwaltung
 *
 * Revision 1.37  2005/05/16 20:37:51  nomenquis
 * added ArchMemory for page table manip
 *
 * Revision 1.36  2005/05/10 21:25:56  nelles
 * 	StackTrace testing
 *
 * Revision 1.35  2005/05/10 19:05:16  nelles
 * changed the panic code to read value directly from ESP
 *
 * Revision 1.34  2005/05/08 21:43:55  nelles
 * changed gcc flags from -g to -g3 -gstabs in order to
 * generate stabs output in object files
 * changed linker script to load stabs in kernel
 * in bss area so GRUB loads them automaticaly with
 * the bss section
 *
 * changed StupidThreads in main for testing purposes
 *
 * Revision 1.33  2005/05/03 18:31:09  btittelbach
 * fix of evil evil MemoryManager Bug
 *
 * Revision 1.32  2005/05/02 21:20:50  nelles
 * added tag to bochs debugwrite
 *
 * Revision 1.31  2005/05/02 21:13:30  nelles
 * added the debug_bochs.h
 *
 * Revision 1.30  2005/05/02 19:58:40  nelles
 * made GetStackPointer in Thread public
 * added panic.cpp
 *
 * Revision 1.29  2005/04/27 08:58:16  nomenquis
 * locks work!
 * w00t !
 *
 * Revision 1.28  2005/04/26 21:38:43  btittelbach
 * Fifo/Pipe Template soweit das ohne Lock und CV zu implementiern ging
 * kprintf kennt jetzt auch chars
 *
 * Revision 1.27  2005/04/26 16:08:59  nomenquis
 * updates
 *
 * Revision 1.26  2005/04/26 15:58:45  nomenquis
 * threads, scheduler, happy day
 *
 * Revision 1.25  2005/04/25 23:23:49  btittelbach
 * nothing really
 *
 * Revision 1.24  2005/04/25 23:09:18  nomenquis
 * fubar 2
 *
 * Revision 1.23  2005/04/25 22:41:58  nomenquis
 * foobar
 *
 * Revision 1.22  2005/04/25 21:15:41  nomenquis
 * lotsa changes
 *
 * Revision 1.21  2005/04/24 20:39:31  nomenquis
 * cleanups
 *
 * Revision 1.18  2005/04/24 16:58:04  nomenquis
 * ultra hack threading
 *
 * Revision 1.16  2005/04/24 10:06:09  nomenquis
 * commit to compile on different machine
 *
 * Revision 1.15  2005/04/23 20:32:30  nomenquis
 * timer interrupt works
 *
 * Revision 1.14  2005/04/23 20:08:26  nomenquis
 * updates
 *
 * Revision 1.13  2005/04/23 17:35:03  nomenquis
 * fixed buggy memory manager
 * (giving out the same memory several times is no good idea)
 *
 * Revision 1.12  2005/04/23 16:03:40  btittelbach
 * kmm testen im main
 *
 * Revision 1.11  2005/04/23 15:58:32  nomenquis
 * lots of new stuff
 *
 * Revision 1.10  2005/04/23 12:52:26  nomenquis
 * fixes
 *
 * Revision 1.9  2005/04/23 12:43:09  nomenquis
 * working page manager
 *
 * Revision 1.8  2005/04/22 20:14:25  nomenquis
 * fix for crappy old gcc versions
 *
 * Revision 1.7  2005/04/22 19:43:04  nomenquis
 *  more poison added
 *
 * Revision 1.6  2005/04/22 18:23:16  nomenquis
 * massive cleanups
 *
 * Revision 1.5  2005/04/22 17:21:41  nomenquis
 * added TONS of stuff, changed ZILLIONS of things
 *
 * Revision 1.4  2005/04/21 21:31:24  nomenquis
 * added lfb support, also we now use a different grub version
 * we also now read in the grub multiboot version
 *
 * Revision 1.3  2005/04/20 08:06:18  nomenquis
 * the overloard (thats me) managed to get paging with 4m pages to work.
 * kernel is now at 2g +1 and writes something to the fb
 * w00t!
 *
 * Revision 1.2  2005/04/20 06:39:11  nomenquis
 * merged makefile, also removed install from default target since it does not work
 *
 * Revision 1.1  2005/04/12 18:42:51  nomenquis
 * changed a zillion of iles
 *
 * Revision 1.1  2005/04/12 17:46:44  nomenquis
 * added lots of files
 *
 *
 */

#include <types.h>
//#include <multiboot.h>
#include <arch_panic.h>
#include <paging-definitions.h>

#include "mm/new.h"
#include "mm/PageManager.h"
#include "mm/KernelMemoryManager.h"
#include "ArchInterrupts.h"
#include "ArchThreads.h"
#include "console/kprintf.h"
#include "Thread.h"
#include "Scheduler.h"
#include "ArchCommon.h"
#include "ArchThreads.h"
#include "kernel/Mutex.h"
#include "panic.h"
#include "debug_bochs.h"
#include "ArchMemory.h"
#include "Loader.h"

#include "arch_serial.h"
#include "drivers/serial.h"

#include "arch_keyboard_manager.h"
#include "atkbd.h"

#include "arch_bd_manager.h"

#include "fs/VirtualFileSystem.h"
#include "fs/ramfs/RamFileSystemType.h"
#include "console/TextConsole.h"
#include "console/Terminal.h"

#include "fs/PseudoFS.h"
#include "fs/fs_tests.h"

#include "TestingThreads.h"

extern void* kernel_end_address;

extern "C" void startup();

class UserThread : public Thread
{
  public:
  UserThread(char *pseudofs_filename, uint32 terminal_number=0)
  {
    name_=pseudofs_filename;
    uint8 *elf_data = PseudoFS::getInstance()->getFilePtr(pseudofs_filename);
    if (elf_data)
    {
      loader_= new Loader(elf_data,this);
      loader_->loadExecutableAndInitProcess();
      run_me_=true;
      terminal_number_=terminal_number;
    }
    else
    {
      run_me_=false;
    }
    kprintf("UserThread::ctor: Done loading %s\n",pseudofs_filename);
  }

  virtual void Run()
  {
    if (run_me_)
      for(;;)
      {
        if (main_console->getTerminal(terminal_number_))
          this->setTerminal(main_console->getTerminal(terminal_number_));          
        kprintf("UserThread:Run: %x %s Going to user, expect page fault\n",this,this->getName());
        this->switch_to_userspace_ = 1;
        Scheduler::instance()->yield();
        //should not reach
      }
    else
      currentThread->kill();
  }

private:
  bool run_me_;
  uint32 terminal_number_;
};


//------------------------------------------------------------
void startup()
{
  writeLine2Bochs( (uint8 *) "Startup Started \n");

  pointer start_address = (pointer)&kernel_end_address;
  pointer end_address = (pointer)(1024U*1024U*1024U*2U + 1024U*1024U*4U); //2GB+4MB Ende des Kernel Bereichs für den es derzeit Paging gibt

  start_address = PageManager::createPageManager(start_address);
  KernelMemoryManager::createMemoryManager(start_address,end_address);
  
  //SerialManager::getInstance()->do_detection( 1 );

  main_console = new TextConsole(8);

  Terminal *term_0 = main_console->getTerminal(0);
  Terminal *term_1 = main_console->getTerminal(1);
  Terminal *term_2 = main_console->getTerminal(2);
  Terminal *term_3 = main_console->getTerminal(3);
  
  term_0->setBackgroundColor(Console::BG_BLACK);
  term_0->setForegroundColor(Console::FG_GREEN);
  term_0->writeString("This is on term 0, you should see me now\n");
  term_1->writeString("This is on term 1, you should not see me, unles you switched to term 1\n");
  term_2->writeString("This is on term 2, you should not see me, unles you switched to term 2\n");
  term_3->writeString("This is on term 3, you should not see me, unles you switched to term 3\n");

  main_console->setActiveTerminal(0);

  kprintfd("Kernel end address is %x and in physical %x\n",&kernel_end_address, ((pointer)&kernel_end_address)-2U*1024*1024*1024+1*1024*1024);

  Scheduler::createScheduler();
  KernelMemoryManager::instance()->startUsingSyncMechanism();
  
  //needs to be done after scheduler and terminal, but prior to enableInterrupts
  kprintf_nosleep_init();
  
  kprintf("Threads init\n");
  ArchThreads::initialise();
  kprintf("Interupts init\n");
  ArchInterrupts::initialise();

  kprintf("Timer enable\n");
  ArchInterrupts::enableTimer();

  ArchInterrupts::enableKBD();
  

  kprintf("Thread creation\n");
  
  kprintfd("Adding Kernel threads\n");
 
  Scheduler::instance()->addNewThread( main_console );
  
  Scheduler::instance()->addNewThread( 
    new TestTerminalThread( "TerminalTestThread", main_console, 1 )
   );
  
  Scheduler::instance()->addNewThread( 
    new BDThread()
    );
    
  kprintfd("Adding UserThreads threads\n");
  //~ Scheduler::instance()->addNewThread(new UserThread("mult.sweb"));
    
  for (uint32 file=0; file < PseudoFS::getInstance()->getNumFiles(); ++ file)
    Scheduler::instance()->addNewThread( 
      new UserThread( PseudoFS::getInstance()->getFileNameByNumber(file))
    ); 
  
  
  Scheduler::instance()->printThreadList();
  
  kprintfd("Now enabling Interrupts...\n");
  kprintf("Now enabling Interrupts...\n");
  //kprintfd_nosleep("Now enabling Interrupts NOSLEEP...\n");
  ArchInterrupts::enableInterrupts();    
    
  kprintfd("Init done\n");
  kprintf("Init done\n");

  Scheduler::instance()->yield();
  
  //Empty Keyboard Buffer so irq1 gets fired
  while (kbdBufferFull()) {
    kprintfd("Emptying Keyboard Port content: %x\n",kbdGetScancode());
  }
  for (;;);
}
