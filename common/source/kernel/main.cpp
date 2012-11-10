/**
 * @file main.cpp
 * starts up SWEB
 */

#include <types.h>

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
#include "assert.h"

#include "arch_serial.h"
#include "drivers/serial.h"

#include "arch_keyboard_manager.h"
#include "atkbd.h"

#include "arch_bd_manager.h"
#include "arch_bd_virtual_device.h"

#include "fs/VfsSyscall.h"
#include "fs/FsWorkingDirectory.h"

#include "console/TextConsole.h"
#include "console/FrameBufferConsole.h"
#include "console/Terminal.h"

#include "fs/fs_global.h"

#include "UserProcess.h"
#include "MountMinix.h"
#include "ustl/outerrstream.h"

#include "backtrace.h"

extern void* kernel_end_address;

extern "C" void startup();

extern unsigned char stab_start_address_nr;
extern unsigned char stab_end_address_nr;

extern unsigned char stabstr_start_address_nr;
extern unsigned char stabstr_end_address_nr;

//HowTo Extend Kernel Memory ?:
// see init_boottime_pagetables.cpp
// then increase end_address appropriately

uint32 boot_completed;

/**
 * startup called in @ref boot.s
 * starts up SWEB
 * Creates singletons, starts console, mounts devices, adds testing threads and start the scheduler.
 */
void startup()
{
  boot_completed = 0;
  writeLine2Bochs ( ( uint8 * ) "SWEB starting...\n" );
  pointer start_address = ArchCommon::getFreeKernelMemoryStart();
  pointer end_address = ArchCommon::getFreeKernelMemoryEnd();
  //extend Kernel Memory here
  KernelMemoryManager::createMemoryManager ( start_address,end_address );
  writeLine2Bochs ( ( uint8 * ) "Kernel Memory Manager created \n" );
  writeLine2Bochs ( ( uint8 * ) "Creating Page Manager...\n" );
  PageManager::createPageManager();
  writeLine2Bochs ( ( uint8 * ) "PageManager created \n" );


  //SerialManager::getInstance()->do_detection( 1 );

#ifdef isXenBuild
  main_console = new XenConsole ( 4 );
  writeLine2Bochs ( ( uint8 * ) "Xen Console created \n" );
#else
  if ( ArchCommon::haveVESAConsole() )
  {
    main_console = new FrameBufferConsole ( 4 );
    writeLine2Bochs ( ( uint8 * ) "Frame Buffer Console created \n" );
  }
  else
  {
    main_console = new TextConsole ( 4 );
    writeLine2Bochs ( ( uint8 * ) "Text Console created \n" );
  }
#endif

  Terminal *term_0 = main_console->getTerminal ( 0 );
  Terminal *term_1 = main_console->getTerminal ( 1 );
  Terminal *term_2 = main_console->getTerminal ( 2 );
  Terminal *term_3 = main_console->getTerminal ( 3 );

  term_0->setBackgroundColor ( Console::BG_BLACK );
  term_0->setForegroundColor ( Console::FG_GREEN );
  kprintfd ( "Init debug printf\n" );
  term_0->writeString ( "This is on term 0, you should see me now\n" );
  term_1->writeString ( "This is on term 1, you should not see me, unless you switched to term 1\n" );
  term_2->writeString ( "This is on term 2, you should not see me, unless you switched to term 2\n" );
  term_3->writeString ( "This is on term 3, you should not see me, unless you switched to term 3\n" );

  main_console->setActiveTerminal ( 0 );

  kprintf ( "Kernel end address is %x and in physical %x\n",&kernel_end_address, ( ( pointer ) &kernel_end_address )-2U*1024*1024*1024+1*1024*1024 );

  Scheduler::createScheduler();

  //needs to be done after scheduler and terminal, but prior to enableInterrupts
  kprintf_nosleep_init();

  debug ( MAIN, "Threads init\n" );
  ArchThreads::initialise();
  debug ( MAIN, "Interupts init\n" );
  ArchInterrupts::initialise();

  parse_symtab((StabEntry*)&stab_start_address_nr, (StabEntry*)&stab_end_address_nr, (const char*)&stabstr_start_address_nr);

  debug ( MAIN, "Block Device creation\n" );
  BDManager::getInstance()->doDeviceDetection( );
  debug ( MAIN, "Block Device done\n" );

  for ( uint32 i = 0; i < BDManager::getInstance()->getNumberOfDevices(); i++ )
  {
    BDVirtualDevice* bdvd = BDManager::getInstance()->getDeviceByNumber ( i );
    debug ( MAIN, "Detected Devices %d: %s :: %d\n",i, bdvd->getName(), bdvd->getDeviceNumber() );

  }

  // initialize global and static objects
  ustl::coutclass::init();
  VfsSyscall::createVfsSyscall();

  extern ustl::list<FileDescriptor*> global_fd;
  new (&global_fd) ustl::list<FileDescriptor*>();
  extern Mutex global_fd_lock;
  new (&global_fd_lock) Mutex("global_fd_lock");

  // the default working directory info
  debug ( MAIN, "creating a default working Directory\n" );
  FsWorkingDirectory default_working_dir;
  debug ( MAIN, "finished with creating working Directory\n" );

  debug ( MAIN, "make a deep copy of FsWorkingDir\n" );
  main_console->setWorkingDirInfo(new FsWorkingDirectory(default_working_dir));
  debug ( MAIN, "main_console->setWorkingDirInfo done\n" );

  debug ( MAIN, "root_fs_info root name: %s\t pwd name: %s\n",
      main_console->getWorkingDirInfo()->getRootDirPath(), main_console->getWorkingDirInfo()->getWorkingDirPath() );

  debug ( MAIN, "Timer enable\n" );
  ArchInterrupts::enableTimer();

  KeyboardManager::getInstance();
  ArchInterrupts::enableKBD();

  debug ( MAIN, "Thread creation\n" );

  debug ( MAIN, "Adding Kernel threads\n" );

  Scheduler::instance()->addNewThread ( main_console );

  // DO NOT CHANGE THE NAME OR THE TYPE OF THE user_progs VARIABLE!
  char const *user_progs[] = {
  // for reasons of automated testing
                              "/stdin-test.sweb",
                              0
                             };

  Scheduler::instance()->addNewThread (
       new MountMinixAndStartUserProgramsThread ( new FsWorkingDirectory(default_working_dir), user_progs )
   );

  Scheduler::instance()->printThreadList();

  kprintf ( "Now enabling Interrupts...\n" );
  boot_completed = 1;
  ArchInterrupts::enableInterrupts();
  
  Scheduler::instance()->yield();

  //not reached
  assert ( false );
}
