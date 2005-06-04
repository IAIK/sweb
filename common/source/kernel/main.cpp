/**
 * $Id: main.cpp,v 1.44 2005/06/04 19:41:26 nelles Exp $
 *
 * $Log: main.cpp,v $
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
#include <multiboot.h>
#include <arch_panic.h>
#include <paging-definitions.h>
#include "console/ConsoleManager.h"
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
#include "ipc/FiFo.h"
#include "kernel/Mutex.h"
#include "panic.h"
#include "debug_bochs.h"
#include "ArchMemory.h"
#include "Loader.h"

#include "arch_serial.h"
#include "drivers/serial.h"

extern void* kernel_end_address;

extern "C" void startup();

Mutex * lock;

class SerialThread : public Thread
{
  public:
    
  SerialThread()
  {
  };
  
  virtual void Run()
  {
    SerialManager *sm = SerialManager::getInstance();
    uint32 num_ports = sm->get_num_ports();
    uint32 i = 0;
    for( i=0; i < num_ports; i++ )
    {
      SerialPort *sp = sm->serial_ports[i];
      kprintf( "Port number : %d, Port name : %s \n", i ,
      sp->friendly_name );
      uint32 bytes_written = 0;
      uint8 buffer2write[] = "TEST";
      sp->write( buffer2write, 4, bytes_written );
      kprintf("bytes_written : %d\n", bytes_written);

      
      // read from serial port and write to console      
      uint8 gotch = 0;
      uint32 num_read = 0;
      
      do
      {
        sp->read( &gotch, 1, num_read );
        if( num_read )
          kprintf( "%c", gotch );
      }
      while( 1 );
      // until forever
    }
    
    kprintf("Done with serial ports\n");
    for(;;) Scheduler::instance()->yield();
  };
  
};


class StupidThread : public Thread
{
  public:
    
  StupidThread(uint32 id)
  {
  //  lock->Acquire();
    thread_number_ = id;
 //   lock->Release();
  }
  
  virtual void Run()
  {
    //uint32 i=0;
    while (1)
    {
 //   kprintf("Thread %d trying to get the lock\n",thread_number_);
      lock->Acquire();
      Scheduler::instance()->yield();
      //kprintf("Thread %d has the lock\n",thread_number_);
  //     kprintf("Kernel Thread %d %d\n",thread_number_,i++);
      lock->Release();
      Scheduler::instance()->yield();
      
     // if( i++ >= 5 )
       // stupid_static_func1( 32  );
        
    }
  }
  
private:
  
  uint32 thread_number_;

};

class UserThread : public Thread
{
  public:
    
  UserThread()
  {
    uint8 *foo=(uint8*)"\177\105\114\106\1\1\1\0\0\0\0\0\0\0\0\0\2\0\3\0\1\0\0\0\264\200\4\10\64\0\0\0\120\1\0\0\0\0\0\0\64\0\40\0\4\0\50\0\10\0\5\0\1\0\0\0\0\0\0\0\0\200\4\10\0\200\4\10\272\0\0\0\272\0\0\0\5\0\0\0\0\20\0\0\1\0\0\0\274\0\0\0\274\220\4\10\274\220\4\10\0\0\0\0\0\0\0\0\6\0\0\0\0\20\0\0\121\345\164\144\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\6\0\0\0\4\0\0\0\200\25\4\145\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\50\0\0\4\0\0\0\125\211\345\220\353\376\0\0\0\107\103\103\72\40\50\107\116\125\51\40\63\56\63\56\65\55\62\60\60\65\60\61\63\60\40\50\107\145\156\164\157\157\40\114\151\156\165\170\40\63\56\63\56\65\56\62\60\60\65\60\61\63\60\55\162\61\54\40\163\163\160\55\63\56\63\56\65\56\62\60\60\65\60\61\63\60\55\61\54\40\160\151\145\55\70\56\67\56\67\56\61\51\0\0\56\163\171\155\164\141\142\0\56\163\164\162\164\141\142\0\56\163\150\163\164\162\164\141\142\0\56\164\145\170\164\0\56\144\141\164\141\0\56\142\163\163\0\56\143\157\155\155\145\156\164\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\33\0\0\0\1\0\0\0\6\0\0\0\264\200\4\10\264\0\0\0\6\0\0\0\0\0\0\0\0\0\0\0\4\0\0\0\0\0\0\0\41\0\0\0\1\0\0\0\3\0\0\0\274\220\4\10\274\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\4\0\0\0\0\0\0\0\47\0\0\0\10\0\0\0\3\0\0\0\274\220\4\10\274\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\4\0\0\0\0\0\0\0\54\0\0\0\1\0\0\0\0\0\0\0\0\0\0\0\274\0\0\0\137\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\0\0\0\0\21\0\0\0\3\0\0\0\0\0\0\0\0\0\0\0\33\1\0\0\65\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\0\0\0\0\1\0\0\0\2\0\0\0\0\0\0\0\0\0\0\0\220\2\0\0\320\0\0\0\7\0\0\0\11\0\0\0\4\0\0\0\20\0\0\0\11\0\0\0\3\0\0\0\0\0\0\0\0\0\0\0\140\3\0\0\47\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\264\200\4\10\0\0\0\0\3\0\1\0\0\0\0\0\274\220\4\10\0\0\0\0\3\0\2\0\0\0\0\0\274\220\4\10\0\0\0\0\3\0\3\0\0\0\0\0\0\0\0\0\0\0\0\0\3\0\4\0\0\0\0\0\0\0\0\0\0\0\0\0\3\0\5\0\0\0\0\0\0\0\0\0\0\0\0\0\3\0\6\0\0\0\0\0\0\0\0\0\0\0\0\0\3\0\7\0\1\0\0\0\0\0\0\0\0\0\0\0\4\0\361\377\10\0\0\0\264\200\4\10\6\0\0\0\22\0\1\0\17\0\0\0\274\220\4\10\0\0\0\0\20\0\361\377\33\0\0\0\274\220\4\10\0\0\0\0\20\0\361\377\42\0\0\0\274\220\4\10\0\0\0\0\20\0\361\377\0\164\145\163\164\56\143\0\137\163\164\141\162\164\0\137\137\142\163\163\137\163\164\141\162\164\0\137\145\144\141\164\141\0\137\145\156\144\0";
    
    Loader loader(foo,this);
    loader.loadExecutableAndInitProcess();
    
    kprintf("Done loading exe \n");
  
  }
  
  virtual void Run()
  {
    for(;;)
    {
      kprintf("Going to userr, expect page fault\n");
      this->switch_to_userspace_ = 1;

      Scheduler::instance()->yield();
    }
  }
  
private:
  
  uint32 bad_mapping_page_0;

};

void startup()
{
  writeLine2Bochs( (uint8 *) "Startup Started \n");

  pointer start_address = (pointer)&kernel_end_address;
  pointer end_address = (pointer)(1024U*1024U*1024U*2U + 1024U*1024U*4U); //2GB+4MB Ende des Kernel Bereichs für den es derzeit Paging gibt
  start_address = PageManager::createPageManager(start_address);
  KernelMemoryManager::createMemoryManager(start_address,end_address);
  ConsoleManager::createConsoleManager(1);
  Scheduler::createScheduler();
  
  Console *console = ConsoleManager::instance()->getActiveConsole();

  console->setBackgroundColor(Console::BG_BLACK);
  console->setForegroundColor(Console::FG_GREEN);

  uint32 dummy = 0;
  kprintf("befor test set lock, val is now %d\n",dummy);
  ArchThreads::testSetLock(dummy,10);
  kprintf("After test set lock, val is now %d\n",dummy);
  kprintf("Lock 2, %d\n",ArchThreads::testSetLock(dummy,22));
  kprintf("After test set lock, val is now %d\n",dummy);
  
 
  kprintf("Threads init\n");
  ArchThreads::initialise();
  kprintf("Interupts init\n");
  ArchInterrupts::initialise();

  kprintf("Timer enable\n");
  ArchInterrupts::enableTimer();
  lock = new Mutex();
  
  kprintf("Serial port initialisation\n");
  SerialManager::getInstance()->do_detection( 1 );
  

  kprintf("Thread creation\n");
  StupidThread *thread0 = new StupidThread(0);
  StupidThread *thread1 = new StupidThread(1);
  
  SerialThread *serial_thread = new SerialThread();
  
  kprintf("Adding threads\n");
  Scheduler::instance()->addNewThread(thread0);
  Scheduler::instance()->addNewThread(thread1);
  
  Scheduler::instance()->addNewThread(serial_thread);
  
  kprintf("now enabling Interrupts...");

  //Scheduler::instance()->addNewThread(new UserThread());
  ArchInterrupts::enableInterrupts();
  kprintf("done\n");
  
  Scheduler::instance()->yield();
  for (;;);
}
