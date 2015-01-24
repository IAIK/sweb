/**
 * @file main.cpp
 * starts up SWEB
 */

#include <ProcessRegistry.h>

#include <types.h>

#include <paging-definitions.h>
#include "ArchSerialInfo.h"
#include "BDManager.h"
#include "BDVirtualDevice.h"

#include "new.h"
#include "PageManager.h"
#include "KernelMemoryManager.h"
#include "ArchInterrupts.h"
#include "ArchThreads.h"
#include "kprintf.h"
#include "Thread.h"
#include "Scheduler.h"
#include "ArchCommon.h"
#include "ArchThreads.h"
#include "Mutex.h"
#include "panic.h"
#include "debug_bochs.h"
#include "ArchMemory.h"
#include "Loader.h"
#include "assert.h"

#include "SerialManager.h"

#include "KeyboardManager.h"
#include "atkbd.h"

#include "VfsSyscall.h"
#include "FsWorkingDirectory.h"

#include "TextConsole.h"
#include "FrameBufferConsole.h"
#include "Terminal.h"

#include "fs_global.h"

#include "UserProcess.h"
#include "outerrstream.h"

#include "user_progs.h"

extern void* kernel_end_address;

extern Console* main_console;

uint8 boot_stack[0x4000] __attribute__((aligned(0x4000)));

uint32 boot_completed;
uint32 we_are_dying;
FsWorkingDirectory default_working_dir;

/**
 * startup called in @ref boot.s
 * starts up SWEB
 * Creates singletons, starts console, mounts devices, adds testing threads and start the scheduler.
 */
extern "C" void removeBootTimeIdentMapping();

extern "C" void startup()
{
  writeLine2Bochs("Removing Boot Time Ident Mapping...\n");
  removeBootTimeIdentMapping();
  we_are_dying = 0;
  boot_completed = 0;
  pointer start_address = ArchCommon::getFreeKernelMemoryStart();
  pointer end_address = ArchCommon::getFreeKernelMemoryEnd();
  //extend Kernel Memory here
  KernelMemoryManager::createMemoryManager(start_address, end_address);
  writeLine2Bochs("Kernel Memory Manager created \n");
  writeLine2Bochs("Creating Page Manager...\n");
  PageManager::createPageManager();
  writeLine2Bochs("PageManager created \n");

  //SerialManager::getInstance()->do_detection( 1 );

  main_console = ArchCommon::createConsole(4);
  writeLine2Bochs("Console created \n");

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

  kprintf("Kernel end address is %x\n", &kernel_end_address);

  Scheduler::createScheduler();

  //needs to be done after scheduler and terminal, but prior to enableInterrupts
  kprintf_init();

  debug ( MAIN, "Threads init\n" );
  ArchThreads::initialise();
  debug ( MAIN, "Interupts init\n" );
  ArchInterrupts::initialise();

  ArchCommon::initDebug();

  debug ( MAIN, "Block Device creation\n" );
  BDManager::getInstance()->doDeviceDetection( );
  debug ( MAIN, "Block Device done\n" );

  for ( uint32 i = 0; i < BDManager::getInstance()->getNumberOfDevices(); i++ )
  {
    BDVirtualDevice* bdvd = BDManager::getInstance()->getDeviceByNumber ( i );
    debug ( MAIN, "Detected Devices %d: %s :: %d\n",i, bdvd->getName(), bdvd->getDeviceNumber() );

  }

  // initialise global and static objects
  ustl::coutclass::init();
  VfsSyscall::createVfsSyscall();

  extern ustl::list<FileDescriptor*> global_fd;
  new (&global_fd) ustl::list<FileDescriptor*>();
  extern Mutex global_fd_lock;
  new (&global_fd_lock) Mutex("global_fd_lock");

  // the default working directory info
  debug ( MAIN, "creating a default working Directory\n" );
  new (&default_working_dir) FsWorkingDirectory();
  debug ( MAIN, "finished with creating working Directory\n" );

  debug ( MAIN, "make a deep copy of FsWorkingDir\n" );
  main_console->setWorkingDirInfo(new FsWorkingDirectory(default_working_dir));
  debug ( MAIN, "main_console->setWorkingDirInfo done\n" );

  debug ( MAIN, "root_fs_info root name: %s\t pwd name: %s\n",
      main_console->getWorkingDirInfo()->getRootDirPath(), main_console->getWorkingDirInfo()->getWorkingDirPath() );

  debug ( MAIN, "Timer enable\n" );
  ArchInterrupts::enableTimer();

  KeyboardManager::instance();
  ArchInterrupts::enableKBD();

  debug ( MAIN, "Thread creation\n" );

  debug ( MAIN, "Adding Kernel threads\n" );

  Scheduler::instance()->addNewThread ( main_console );

  Scheduler::instance()->addNewThread (
       new ProcessRegistry ( new FsWorkingDirectory(default_working_dir), user_progs ) // see user_progs.h
   );

  Scheduler::instance()->printThreadList();

  kprintf ( "Now enabling Interrupts...\n" );
  boot_completed = 1;
  ArchInterrupts::enableInterrupts();

  Scheduler::instance()->yield();

  //not reached
  assert ( false );
}
