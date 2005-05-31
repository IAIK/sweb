//----------------------------------------------------------------------
//   $Id: Scheduler.cpp,v 1.5 2005/05/31 17:25:56 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: Scheduler.cpp,v $
//  Revision 1.4  2005/05/25 08:27:49  nomenquis
//  cr3 remapping finally really works now
//
//  Revision 1.3  2005/05/08 21:43:55  nelles
//  changed gcc flags from -g to -g3 -gstabs in order to
//  generate stabs output in object files
//  changed linker script to load stabs in kernel
//  in bss area so GRUB loads them automaticaly with
//  the bss section
//
//  changed StupidThreads in main for testing purposes
//
//  Revision 1.2  2005/04/27 08:58:16  nomenquis
//  locks work!
//  w00t !
//
//  Revision 1.1  2005/04/26 15:58:45  nomenquis
//  threads, scheduler, happy day
//
//----------------------------------------------------------------------


#include "Scheduler.h"
#include "Thread.h"
#include "arch_panic.h"
#include "ArchThreads.h"
#include "console/kprintf.h"

ArchThreadInfo *currentThreadInfo;
Thread *currentThread;

 
Scheduler *Scheduler::instance_=0;

Scheduler *Scheduler::instance()
{
  return instance_;
}

class IdleThread : public Thread
{
public:
  
  virtual void Run()
  {
    while (1)
    {
//      kprintf("IDLE\n");
      Scheduler::instance()->yield();
    }
  }
};
void Scheduler::createScheduler()
{
  instance_ = new Scheduler();
  
  // create idle thread, this one really does not do too much

  Thread *idle = new IdleThread();
  instance_->addNewThread(idle);
}

Scheduler::Scheduler()
{
  //~ uint32 i;
  //~ for (i=0;i<MAX_THREADS;++i)
    //~ threads_[i] = 0;
}

void Scheduler::addNewThread(Thread *thread)
{
  threads_.pushBack(thread);
  
  //~ uint32 i;
  
  //~ for (i=0;i<MAX_THREADS;++i)
  //~ {
    //~ if (!threads_[i])
    //~ {
      //~ threads_[i] = thread;
      //~ break;
    //~ }
  //~ }
  //~ if (i==MAX_THREADS)
  //~ {
    //~ arch_panic((uint8*)"Too many threads\n");
  //~ }
  
}

void Scheduler::startThreadHack()
{
  currentThread->Run();
}

uint32 Scheduler::schedule(uint32 from_interrupt)
{
  static uint32 bochs_sucks = 0;
  if (++bochs_sucks % 1)
  {
    return 0;
  }
  //~ uint32 i;
  //~ for (i=0;i<MAX_THREADS;++i)
  //~ {
    //~ if (currentThread == threads_[i])
    //~ {
      //~ break;
    //~ }
  //~ }
  //~ uint32 found = 0;
  //~ while (!found)
  //~ {
    //~ i = (i+1)%MAX_THREADS;
    //~ if (threads_[i])
      //~ break;
  //~ }
  //~ uint32 ret = 1;
  
 /*
  if (currentThread != 0 && 
    currentThread->page_directory_ != threads_[i]->page_directory_)
  {
      ret = 1;
  }
  if (currentThread)
    kprintf("Pagedir %x %x\n",currentThread->page_directory_, threads_[i]->page_directory_);
  */
  
  //kprintf("Thread Switch: previous Thread %x\n",currentThread);
  currentThread = threads_.front();
  //kprintf("Thread Switch: next Thread %x\n",currentThread);
  threads_.popFront();
  threads_.pushBack(currentThread);
  currentThreadInfo = currentThread->arch_thread_info_;
  return 1;
  
  //~ currentThread = threads_[i];
  //~ currentThreadInfo = threads_[i]->arch_thread_info_;
  //~ return ret;
}

void Scheduler::yield()
{
  ArchThreads::yield();
}
