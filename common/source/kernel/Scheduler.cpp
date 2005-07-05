//----------------------------------------------------------------------
//   $Id: Scheduler.cpp,v 1.10 2005/07/05 20:22:56 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: Scheduler.cpp,v $
//  Revision 1.9  2005/07/05 17:29:48  btittelbach
//  new kprintf(d) Policy:
//  [Class::]Function: before start of debug message
//  Function can be abbreviated "ctor" if Constructor
//  use kprintfd where possible
//
//  Revision 1.8  2005/06/14 18:51:47  btittelbach
//  afterthought page fault handling
//
//  Revision 1.7  2005/05/31 18:13:14  nomenquis
//  fixed compile errors
//
//  Revision 1.6  2005/05/31 17:29:16  nomenquis
//  userspace
//
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
}

void Scheduler::addNewThread(Thread *thread)
{
  //new Thread gets scheduled next
  //also gets added to front as not to interfere with remove or xchange
  threads_.pushFront(thread);
}


//you can't remove the last thread
void Scheduler::removeCurrentThread()
{
  if (threads_.size() > 1)
  {
    //we can safely do this, because it won't affect the thread switch to the next thread
    //just ensure that the currentThread in the list never again gets scheduled
    //don't do this if you still need to clean up some things
    
    threads_.popBack();
  }
}


//exchanges the current Thread with a PopUpThread
//note that the popupthread has to remember the original Thread*
//and switch back if he's finished
Thread *Scheduler::xchangeThread(Thread *pop_up_thread)
{
  Thread *old_thread = threads_.back();
  if (old_thread != currentThread)
  kprintf("Scheduler::xchangeThread: ERROR: currentThread wasn't where it was supposed to be\n");
  threads_.popBack();
  threads_.pushBack(pop_up_thread);
  currentThread=pop_up_thread;
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

  uint32 ret = 1;
  currentThread = threads_.front();
  threads_.popFront();
  threads_.pushBack(currentThread);
  if ( currentThread->switch_to_userspace_)
    currentThreadInfo =  currentThread->user_arch_thread_info_;
  else
    currentThreadInfo =  currentThread->kernel_arch_thread_info_, ret=0;
  /*uint8*foo = 0;
  *foo = 8;*/
  return ret;

}

void Scheduler::yield()
{
  ArchThreads::yield();
}
