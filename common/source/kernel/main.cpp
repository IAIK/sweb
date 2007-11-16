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

#include "fs/VirtualFileSystem.h"
#include "fs/ramfs/RamFSType.h"
#include "fs/devicefs/DeviceFSType.h"
#include "fs/minixfs/MinixFSType.h"
#include "console/TextConsole.h"
#include "console/FrameBufferConsole.h"
#include "console/Terminal.h"
#include "XenConsole.h"

#include "fs/PseudoFS.h"
#include "fs/fs_tests.h"

#include "fs/fs_global.h"


extern void* kernel_end_address;

extern "C" void startup();

/**
 * @class UserThread
 * Thread used to execute a file in pseudofs.
 */
class UserThread : public Thread
{
  public:
    /**
     * Constructor
     * @param pseudofs_filename filename of the file in pseudofs to execute
     * @param terminal_number the terminal to run in (default 0)
     */
    UserThread ( char *pseudofs_filename, FileSystemInfo *fs_info, uint32 terminal_number=0 ) : Thread ( fs_info )
    {
      kprintfd ( "UserThread::ctor: starting %s\n",pseudofs_filename );
      name_=pseudofs_filename;
      uint8 *elf_data = PseudoFS::getInstance()->getFilePtr ( pseudofs_filename );
      if ( elf_data )
      {
        loader_= new Loader ( elf_data,this );
        loader_->loadExecutableAndInitProcess();
        run_me_=true;
        terminal_number_=terminal_number;
      }
      else
      {
        run_me_=false;
      }
      kprintf ( "UserThread::ctor: Done loading %s\n",pseudofs_filename );
      kprintfd ( "UserThread::ctor: Done loading %s\n",pseudofs_filename );
    }

    /**
     * Starts the thread
     */
    virtual void Run()
    {
      if ( run_me_ )
        for ( ;; )
        {
          if ( main_console->getTerminal ( terminal_number_ ) )
            this->setTerminal ( main_console->getTerminal ( terminal_number_ ) );
          kprintf ( "UserThread:Run: %x  %d:%s Going to user, expect page fault\n",this,this->getPID(),this->getName() );
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

/**
 * @class MinixUserThread
 * Thread used to execute a file in minixfs.
 */
class MinixUserThread : public Thread
{
  public:
    /**
     * Constructor
     * @param minixfs_filename filename of the file in minixfs to execute
     * @param terminal_number the terminal to run in (default 0)
     */
    MinixUserThread ( char *minixfs_filename, FileSystemInfo *fs_info, uint32 terminal_number=0 ) : Thread ( fs_info )
    {
      name_= minixfs_filename;
      int32 fd = vfs_syscall.open ( minixfs_filename,0 );
      if ( fd < 0 )
      {
        run_me_ = false;
        kprintf ( "Error: file %s does not exist!\n",minixfs_filename );
        return;
      }
      uint32 file_size = vfs_syscall.getFileSize ( fd );
      char *elf_data = new char[file_size];

      if ( vfs_syscall.read ( fd,elf_data,file_size ) )
      {
        loader_= new Loader ( ( uint8 * ) elf_data,this );
        loader_->loadExecutableAndInitProcess();
        run_me_=true;
        terminal_number_=terminal_number;
      }
      else
      {
        run_me_=false;
      }
      kprintf ( "MinixUserThread::ctor: Done loading %s\n",minixfs_filename );
      delete elf_data;
    }

    /**
     * Starts the thread
     */
    virtual void Run()
    {
      if ( run_me_ )
        for ( ;; )
        {
          if ( main_console->getTerminal ( terminal_number_ ) )
            this->setTerminal ( main_console->getTerminal ( terminal_number_ ) );
          kprintf ( "MinixUserThread:Run: %x  %d:%s Going to user, expect page fault\n",this,this->getPID(), this->getName() );
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

#include "TestingThreads.h"

/**
 * @class TestThread
 * Thread starting all testing threads.
 */
class TestThread : public Thread
{
  public:
    /**
     * Constructor
     */
    TestThread()
    {
      name_="TestThread";
    }

    /**
     * Runs all testing threads
     */
    virtual void Run()
    {
      kprintfd ( "TestThread: running\n" );
      Scheduler::instance()->yield();
      kprintfd ( "TestThread: adding BDThread\n" );
      Scheduler::instance()->addNewThread (
          new BDThread()
      );
      kprintfd ( "TestThread: done adding BDThread\n" );
      Scheduler::instance()->yield();
      kprintfd ( "TestThread: adding SerialTestThread\n" );
      Scheduler::instance()->addNewThread (
          new SerialThread ( "SerialTestThread" )
      );
      kprintfd ( "TestThread: done adding SerialTestThread\n" );
      Scheduler::instance()->yield();
      kprintfd ( "TestThread: adding DeviceFSMountingThread\n" );
      Scheduler::instance()->addNewThread (
          new DeviceFSMountingThread()
      );
      kprintfd ( "TestThread: done adding DeviceFSMountingThread\n" );
      Scheduler::instance()->yield();
      kprintfd ( "\nDone Adding Threads\n" );
      kprintfd ( "\n\n" );
    }
};


/**
 * startup called in @ref boot.s
 * starts up SWEB
 * Creates singletons, starts console, mounts devices, adds testing threads and start the scheduler.
 */
void startup()
{
  pointer start_address = ArchCommon::getFreeKernelMemoryStart();
  //pointer end_address = (pointer)(1024U*1024U*1024U*2U + 1024U*1024U*4U); //2GB+4MB Ende des Kernel Bereichs für den es derzeit Paging gibt
  pointer end_address = ArchCommon::getFreeKernelMemoryEnd();

  writeLine2Bochs ( ( uint8 * ) "Creating Page Manager\n" );
  start_address = PageManager::createPageManager ( start_address );
  writeLine2Bochs ( ( uint8 * ) "PageManager created \n" );
  KernelMemoryManager::createMemoryManager ( start_address,end_address );
  writeLine2Bochs ( ( uint8 * ) "Kernel Memory Manager created \n" );
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
  //writeLine2Bochs( (uint8 *) "Console created \n");

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


  debug ( MAIN, "Mounting DeviceFS under /dev/\n" );
  DeviceFSType *devfs = new DeviceFSType();
  vfs.registerFileSystem ( devfs );
  FileSystemInfo* root_fs_info = vfs.root_mount ( "devicefs", 0 );
//   kprintfd("Mount returned %d\n", mntres);

  debug ( MAIN, "root_fs_info : %d",root_fs_info );
  debug ( MAIN, "root_fs_info root name: %s\t pwd name: %s\n", root_fs_info->getRoot()->getName(), root_fs_info->getPwd()->getName() );
  if ( main_console->getFSInfo() )
  {
    delete main_console->getFSInfo();
  }
  main_console->setFSInfo ( root_fs_info );

  Scheduler::createScheduler();

  //needs to be done after scheduler and terminal, but prior to enableInterrupts
  kprintf_nosleep_init();

  debug ( MAIN, "Threads init\n" );
  ArchThreads::initialise();
  debug ( MAIN, "Interupts init\n" );
  ArchInterrupts::initialise();

  debug ( MAIN, "Block Device creation\n" );
  BDManager::getInstance()->doDeviceDetection( );
  debug ( MAIN, "Block Device done\n" );

  for ( uint32 i = 0; i < BDManager::getInstance()->getNumberOfDevices(); i++ )
  {
    BDVirtualDevice* bdvd = BDManager::getInstance()->getDeviceByNumber ( i );
    debug ( MAIN, "Detected Devices %d: %s :: %d\n",i, bdvd->getName(), bdvd->getDeviceNumber() );

  }

  debug ( MAIN, "Timer enable\n" );
  ArchInterrupts::enableTimer();

  ArchInterrupts::enableKBD();

  debug ( MAIN, "Thread creation\n" );

  debug ( MAIN, "Adding Kernel threads\n" );

  Scheduler::instance()->addNewThread ( main_console );

//   Scheduler::instance()->addNewThread (
//       new MinixTestingThread ( new FileSystemInfo ( *root_fs_info ) )
//   );


//   Scheduler::instance()->addNewThread(
//     new TestTerminalThread( "TerminalTestThread", main_console, 1 )
//   );

//   Scheduler::instance()->addNewThread(
//     new BDThread()
//   );
//
//   Scheduler::instance()->addNewThread(
//      new BDThread2()
//    );

  //~ Scheduler::instance()->addNewThread(
  //~ new BDThread()
  //~ );

  //~ Scheduler::instance()->addNewThread(
  //~ new BDThread2()
  //~ );

  //~ Scheduler::instance()->addNewThread(
  //~ new SerialThread( "SerialTestThread" )
  //~ );

  //~ Scheduler::instance()->addNewThread(
  //~ new DeviceFSMountingThread()
  //~ );

  //~ Scheduler::instance()->addNewThread(new UserThread("mult.sweb"));

  for ( uint32 file=0; file < PseudoFS::getInstance()->getNumFiles(); ++ file )
  {
    UserThread *user_thread = new UserThread ( PseudoFS::getInstance()->getFileNameByNumber ( file ), new FileSystemInfo ( *root_fs_info ) );
    Scheduler::instance()->addNewThread ( user_thread );
  }

  //Scheduler::instance()->addNewThread(new TestThread());

  Scheduler::instance()->printThreadList();

  PageManager::instance()->startUsingSyncMechanism();
  KernelMemoryManager::instance()->startUsingSyncMechanism();

  kprintf ( "Now enabling Interrupts...\n" );
  ArchInterrupts::enableInterrupts();

  Scheduler::instance()->yield();

  //not reached
  assert ( false );
}
