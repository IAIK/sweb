//----------------------------------------------------------------------
//   $Id: Scheduler.cpp,v 1.13 2005/07/24 17:02:59 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: Scheduler.cpp,v $
//  Revision 1.12  2005/07/21 19:08:41  btittelbach
//  Jö schön, Threads u. Userprozesse werden ordnungsgemäß beendet
//  Threads können schlafen, Mutex benutzt das jetzt auch
//  Jetzt muß nur der Mutex auch überall verwendet werden
//
//  Revision 1.11  2005/07/12 21:05:38  btittelbach
//  Lustiges Spielen mit UserProgramm Terminierung
//
//  Revision 1.10  2005/07/05 20:22:56  btittelbach
//  some changes
//
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
#include "ArchInterrupts.h"

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
      Scheduler::instance()->cleanupDeadThreads(); 
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
  kill_me_=0;
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

void Scheduler::sleep()
{
  if (threads_.size() > 1)
  { 
    currentThread->state_=Sleeping;
    yield();
  }
}

void Scheduler::wake(Thread* thread_to_wake)
{
  thread_to_wake->state_=Running;
  //DEBUG: Check if thread_to_wake is in List
}

//exchanges the current Thread with a PopUpThread
//note that the popupthread has to remember the original Thread*
//and switch back if he's finished
Thread *Scheduler::xchangeThread(Thread *pop_up_thread)
{
  Thread *old_thread = threads_.back();
  if (old_thread != currentThread)
    arch_panic((uint8*)"Scheduler::xchangeThread: ERROR: currentThread wasn't where it was supposed to be\n");
  threads_.popBack();
  threads_.pushBack(pop_up_thread);
  currentThread=pop_up_thread;
  return old_thread;
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
  
  do {
    currentThread = threads_.front();
    threads_.popFront();
    
    if (kill_me_ == 0 && currentThread->state_ == ToBeDestroyed)
    {
      kill_me_=currentThread;
      //to be killed Thread gets implicitly removed from list
      continue;
    }
    
    threads_.pushBack(currentThread);
    
  } while (currentThread->state_ != Running);
  
  uint32 ret = 1;
  
  if ( currentThread->switch_to_userspace_)
    currentThreadInfo =  currentThread->user_arch_thread_info_;
  else
  {
    currentThreadInfo =  currentThread->kernel_arch_thread_info_;
    ret=0;
  }
  /*uint8*foo = 0;
  *foo = 8;*/
  return ret;
}

void Scheduler::yield()
{
  ArchThreads::yield();
}

void Scheduler::cleanupDeadThreads()
{
  //check outside of atmoarity for performance gain,
  // worst case, dead threads are around a while longer
  //then make sure we're atomar (can't really lock list, can I ;->)
  //note: currentThread is always last on list

  if (kill_me_ == 0)
    return;
  
  ArchInterrupts::disableInterrupts();
//  kprintfd("Scheduler::cleanupDeadThreads: now running\n");
  if (kill_me_)
  {
    if (kill_me_->state_ == ToBeDestroyed)
      delete kill_me_;  
//    else
//      kprintfd("Scheduler::cleanupDeadThreads: ERROR, how did that Thread get to be here\n");
    kill_me_=0;
  }
//  kprintfd("Scheduler::cleanupDeadThreads: done\n");
  ArchInterrupts::enableInterrupts();
}
