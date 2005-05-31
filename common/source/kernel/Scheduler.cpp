//----------------------------------------------------------------------
//   $Id: Scheduler.cpp,v 1.6 2005/05/31 17:29:16 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: Scheduler.cpp,v $
//  Revision 1.5  2005/05/31 17:25:56  btittelbach
//  Scheduler mit Listen geschmückt
//
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


  currentThread = threads_.front();
  threads_.popFront();
  threads_.pushBack(currentThread);
  if ( currentThread->switch_to_userspace_)
    currentThreadInfo =  currentThread->user_arch_thread_info_;
  else
    currentThreadInfo =  currentThread->kernel_arch_thread_info_,ret=0;
  /*uint8*foo = 0;
  *foo = 8;*/
  return ret;

}

void Scheduler::yield()
{
  ArchThreads::yield();
}
