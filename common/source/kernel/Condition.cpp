//----------------------------------------------------------------------
//   $Id: Condition.cpp,v 1.1 2005/09/07 00:33:52 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: Condition.cpp,v $
//
//----------------------------------------------------------------------

#include "Condition.h"
#include "Scheduler.h"
#include "assert.h"
#include "console/kprintf.h"

Condition::Condition(Mutex *lock)
{
  sleepers_=new List<Thread *>();
  lock_=lock;
}
Condition::~Condition()
{
  delete sleepers_;
}

void Condition::wait()
{
  // list is protected, because we assume, the lock is being held
  assert(lock_->isHeldBy(currentThread));
  sleepers_->pushBack(currentThread);
  lock_->release();
  //<-- an interrupt and signal could happen here or during "sleep()"  ! problem: Thread* gets deleted before thread goes to sleep -> no wakeup call possible on next signal
  kprintfd("Condition::wait: Thread %x %s wating on Condition %x\n",currentThread,currentThread->getName(),this);
  Scheduler::instance()->sleep();
  lock_->acquire();
}

void Condition::signal()
{
  if (! lock_->isHeldBy(currentThread))
    return;
  Thread *thread=0;
  if (!sleepers_->empty())
  {
    thread = sleepers_->front();
    if (thread->state_ == Sleeping)
    {
      //Solution to above Problem: Wake and Remove from List only Threads which are actually sleeping
      Scheduler::instance()->wake(thread);
      sleepers_->popFront();
    }
  }
  if (thread)
    kprintfd("Condition::signal: Thread %x %s being signaled for Condition %x\n",thread,thread->getName(),this);
}

void Condition::broadcast()
{
  if (! lock_->isHeldBy(currentThread))
    return;
  Thread *thread;
  List<Thread*> tmp_threads;
  while (!sleepers_->empty())
  {
    thread = sleepers_->front();
    sleepers_->popFront();
    if (thread->state_ == Sleeping)
      Scheduler::instance()->wake(thread);
    else
      tmp_threads.pushBack(thread);
    kprintfd("Condition::broadcast: Thread %x %s being signaled for Condition %x\n",thread,thread->getName(),this);
  }
  while (!tmp_threads.empty())
  {
    sleepers_->pushBack(tmp_threads.front());
    tmp_threads.popFront();
  }
}
