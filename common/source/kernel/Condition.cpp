/**
 * @file Condition.cpp
 */

#include "Condition.h"
#include "Scheduler.h"
#include "assert.h"
#include "console/kprintf.h"
#include "ArchInterrupts.h"
#include "console/debug.h"

Condition::Condition(Mutex *lock)
{
  sleepers_=new ustl::list<Thread *>();
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
  assert(ArchInterrupts::testIFSet());
  sleepers_->push_back(currentThread);
  //<-- an interrupt and signal could happen here or during "sleep()"  ! problem: Thread* gets deleted before thread goes to sleep -> no wakeup call possible on next signal
  debug(CONDITION, "Condition::wait: Thread %x  %d:%s wating on Condition %x\n",currentThread,currentThread->getPID(),currentThread->getName(),this);
  Scheduler::instance()->sleepAndRelease(*lock_);
  lock_->acquire();
}

void Condition::signal()
{
  if (! lock_->isHeldBy(currentThread))
    return;
  assert(ArchInterrupts::testIFSet());
  Thread *thread=0;
  if (!sleepers_->empty())
  {
    thread = sleepers_->front();
    if (thread->state_ == Sleeping)
    {
      //Solution to above Problem: Wake and Remove from List only Threads which are actually sleeping
      Scheduler::instance()->wake(thread);
      sleepers_->pop_front();
    }
  }
  if (thread)
    debug(CONDITION,"Condition::signal: Thread %x  %d:%s being signaled for Condition %x\n",thread,thread->getPID(),thread->getName(),this);
}

void Condition::broadcast()
{
  if (! lock_->isHeldBy(currentThread))
    return;
  assert(ArchInterrupts::testIFSet());
  Thread *thread;
  ustl::list<Thread*> tmp_threads;
  while (!sleepers_->empty())
  {
    thread = sleepers_->front();
    sleepers_->pop_front();
    if (thread->state_ == Sleeping)
      Scheduler::instance()->wake(thread);
    else
      tmp_threads.push_back(thread);
    debug(CONDITION,"Condition::broadcast: Thread %x  %d:%s being signaled for Condition %x\n",thread,thread->getPID(),thread->getName(),this);
  }
  while (!tmp_threads.empty())
  {
    Thread* temp = tmp_threads.front();
    sleepers_->push_back(temp);
    tmp_threads.pop_front();
  }
}
