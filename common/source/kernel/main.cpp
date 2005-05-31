/**
 * $Id: main.cpp,v 1.40 2005/05/31 13:23:25 nelles Exp $
 *
 * $Log: main.cpp,v $
 * Revision 1.39  2005/05/25 08:27:49  nomenquis
 * cr3 remapping finally really works now
 *
 * Revision 1.38  2005/05/19 15:43:43  btittelbach
 * Ansätze für eine UserSpace Verwaltung
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

extern void* kernel_end_address;

extern "C" void startup();

Mutex * lock;

static void stupid_static_func3( uint8 param )
{
  kpanict( (uint8 *) " panicking " );
  kprintf("f3");
  return;
};

static void stupid_static_func2( uint16 param )
{
  stupid_static_func3( 8 );
  kprintf("f2");
  return;
};

static void stupid_static_func1( uint32 param )
{
  stupid_static_func2( 12 );
  kprintf("f1");
  return;
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
    uint32 i=0;
    while (1)
    {
      kprintf("Thread %d trying to get the lock\n",thread_number_);
    lock->Acquire();
    Scheduler::instance()->yield();
      kprintf("Thread %d has the lock\n",thread_number_);
      kprintf("Kernel Thread %d %d\n",thread_number_,i++);
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
    uint32 page_for_page_dir = PageManager::instance()->getFreePhysicalPage();
    kprintf("Got new Page no. %d\n",page_for_page_dir);
   // page_directory_ = page_for_page_dir * PAGE_SIZE;
    ArchMemory::initNewPageDirectory(page_for_page_dir);
    kprintf("Initialised the page dir\n");
    
    uint32 i;
    for (uint32 i=0;i<20;++i)
    {
      uint32 page = PageManager::instance()->getFreePhysicalPage();
      ArchMemory::mapPage(page_for_page_dir,i,page,1);
      if (!i) kprintf("Mapped a page to the userspace page dir %x\n",page);
      pointer memory = ArchMemory::physicalPageToKernelPointer(page);
      ArchCommon::bzero(memory,PAGE_SIZE,0);
      uint32 k;
      for (k=0;k<1024;++k)
      {
        uint32 *m= (uint32*)memory;
        m[k] = i+100000000;
      }
      
      if (!i)  kprintf("Done with bezero\n");
      if (i==0)
        bad_mapping_page_0 = page*PAGE_SIZE;
    }
    ArchThreads::setPageDirectory(this,page_for_page_dir);
    kprintf("Pageforpagedir is %x\n",page_for_page_dir);
  }
  
  virtual void Run()
  {
    for(;;)
    {
      kprintf("BLuna\n");
      kprintf("%x == %x %x == %x\n",currentThread,this,currentThreadInfo,this->arch_thread_info_);
      uint32* k=(uint32*)(0);
      
      uint32 i;
      for (i=0;i<400000;++i)
        kprintf("k[%d, %d]=%d\n",i,i/1024,k[i]);
      for (;;);
      *k='B';++k;
      *k='L';++k;
      *k='U';++k;
      *k='N';++k;
      *k='A';++k;
      *k='B';++k;
      *k='L';++k;
      *k='U';++k;
      *k='B';++k;
      *k='B';++k;
      *k='\0';++k;
           
      uint32* j=(uint32*)(bad_mapping_page_0+3U*1024U*1024U*1024U);
      for(;;);
      while (1)
      {
        if (*j == 'B' && j[1] == 'L' && j[2] == 'U')
        {
          kprintf("W00t %x\n",j);
          for(;;);
          break;
        }
        else
        {
          kprintf(":(\n");
          ++j;
        }
      }
      /*
      kprintf("J is %x\n",j);
      if (*j == 42)
      {
        kprintf("goodie\n");
      }
      else
      {
        kprintf("Baddie %x != %x\n",*j,*k);
        kprintf("%x %x \n",((pointer)j)-(3*1024*1024*1024)-((pointer)k),(pointer)j);
    }
      */
      Scheduler::instance()->yield();
    }
  }
  
private:
  
  uint32 bad_mapping_page_0;

};

void startup()
{
  writeLine2Bochs( (uint8 *) "It's easy to write in Bochs \n");
  writeLine2Bochs( (uint8 *) "Startup Started \n");

  pointer start_address = (pointer)&kernel_end_address;
  pointer end_address = (pointer)(1024U*1024U*1024U*2U + 1024U*1024U*4U); //2GB+4MB Ende des Kernel Bereichs fÃ¼r den es derzeit Paging gibt
  start_address = PageManager::createPageManager(start_address);
  KernelMemoryManager::createMemoryManager(start_address,end_address);
  ConsoleManager::createConsoleManager(1);
  Scheduler::createScheduler();

  
  Console *console = ConsoleManager::instance()->getActiveConsole();

  console->setBackgroundColor(Console::BG_BLACK);
  console->setForegroundColor(Console::FG_GREEN);

    
 /* console->writeString((uint8 const*)"Blabb\n");  
  console->writeString((uint8 const*)"Blubb sagte die Katze und frasz den Hund\n");
  console->writeString((uint8 const*)"Noch ne Zeile\n");
  console->writeString((uint8 const*)"Und jetzt ne leere\n\n");
  console->writeString((uint8 const*)"Gruen rackete autobus\n");
  console->writeString((uint8 const*)"LAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAANNNNNNNNNNNNNNNNNNNNGEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEERRRRRRRRRRRRRRRRR STRING\n");*/

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

  Scheduler::instance()->addNewThread(new UserThread());
  ArchInterrupts::enableInterrupts();
  kprintf("done\n");
  
  Scheduler::instance()->yield();
  for (;;);
}
