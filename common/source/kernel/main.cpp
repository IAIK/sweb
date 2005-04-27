/**
 * $Id: main.cpp,v 1.29 2005/04/27 08:58:16 nomenquis Exp $
 *
 * $Log: main.cpp,v $
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
 
#include "types.h"
#include "multiboot.h"
#include "arch_panic.h"
#include "paging-definitions.h"
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
#include "ipc/FiFo.h"
#include "kernel/Mutex.h"

extern void* kernel_end_address;

extern "C" void startup();

Mutex * lock;

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
    uint32 i=0;
    while (1)
    {
      kprintf("Thread %d trying to get the lock\n",thread_number_);
    lock->Acquire();
    Scheduler::instance()->yield();
      kprintf("Thread %d has the lock\n",thread_number_);
      kprintf("Kernel Thread %d %d\n",thread_number_,i++);
    lock->Release();
    }
  }
  
private:
  
  uint32 thread_number_;

};


void startup()
{
  
  pointer start_address = (pointer)&kernel_end_address;
  pointer end_address = (pointer)(1024U*1024U*1024U*2U + 1024U*1024U*4U);
  start_address = PageManager::createPageManager(start_address);
  KernelMemoryManager::createMemoryManager(start_address,end_address);
  ConsoleManager::createConsoleManager(1);
  Scheduler::createScheduler();


  Console *console = ConsoleManager::instance()->getActiveConsole();

  console->setBackgroundColor(Console::BG_BLACK);
  console->setForegroundColor(Console::FG_GREEN);

  
  console->writeString((uint8 const*)"Blabb\n");  
  console->writeString((uint8 const*)"Blubb sagte die Katze und frasz den Hund\n");
  console->writeString((uint8 const*)"Noch ne Zeile\n");
  console->writeString((uint8 const*)"Und jetzt ne leere\n\n");
  console->writeString((uint8 const*)"Gruen rackete autobus\n");
  console->writeString((uint8 const*)"LAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAANNNNNNNNNNNNNNNNNNNNGEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEERRRRRRRRRRRRRRRRR STRING\n");

  uint32 dummy = 0;
  kprintf("befor test set lock, val is now %d\n",dummy);
  ArchThreads::testSetLock(dummy,10);
  kprintf("After test set lock, val is now %d\n",dummy);
  kprintf("Lock 2, %d\n",ArchThreads::testSetLock(dummy,22));
  kprintf("After test set lock, val is now %d\n",dummy);
  
  lock = new Mutex();

  //~ kprintf("Testing FiFo:\n");
  //~ FiFo<uint8> wuffwuff(6);
  //~ kprintf("putting 5 elements:\n");
  //~ wuffwuff.put((uint8) 'a');
  //~ wuffwuff.put((uint8) 'b');
  //~ wuffwuff.put((uint8) 'c');
  //~ wuffwuff.put((uint8) 'd');
  //~ wuffwuff.put((uint8) 'e');
  
  //~ kprintf("getting 4 elements: %c ",wuffwuff.get());
  //~ kprintf("%c ",wuffwuff.get());
  //~ kprintf("%c ",wuffwuff.get());
  //~ kprintf("%c \n",wuffwuff.get());

  //~ kprintf("putting 3 more elements:\n");
  //~ wuffwuff.put((uint8) 'f');
  //~ wuffwuff.put((uint8) 'g');
  //~ wuffwuff.put((uint8) 'h');
  //~ kprintf("getting 4 elements: %c ",wuffwuff.get());
  //~ kprintf("%c ",wuffwuff.get());
  //~ kprintf("%c ",wuffwuff.get());
  //~ kprintf("%c \n",wuffwuff.get());
  //~ kprintf("getting 4 more, expecting to block, bye bye:");
  //~ kprintf("%c",wuffwuff.get());
  //~ kprintf("%c",wuffwuff.get());
  //~ kprintf("%c",wuffwuff.get());
  //~ kprintf("%c\n",wuffwuff.get());
  //~ kprintf("OMG, we survived, something is wrong");
 
  kprintf("Threads init\n");
  ArchThreads::initialise();
  kprintf("Interupts init\n");
  ArchInterrupts::initialise();

  kprintf("Timer enable\n");
  ArchInterrupts::enableTimer();
  lock = new Mutex();

  kprintf("Thread creation\n");
  StupidThread *thread0 = new StupidThread(0);
  StupidThread *thread1 = new StupidThread(1);
  kprintf("Adding threads\n");
  Scheduler::instance()->addNewThread(thread0);
  Scheduler::instance()->addNewThread(thread1);
  
  kprintf("now enabling Interrupts...");

/*
  kprintf("Trying to get the lock\n");
  lock->Acquire();
  kprintf("Done\n");
  */

  ArchInterrupts::enableInterrupts();
  kprintf("done\n");

  
  Scheduler::instance()->yield();
  for (;;);
}
