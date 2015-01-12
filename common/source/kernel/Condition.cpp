/**
 * @file Condition.cpp
 */

#include "Condition.h"
#include "Thread.h"
#include "Mutex.h"
#include "Scheduler.h"
#include "assert.h"
#include "kprintf.h"
#include "ArchInterrupts.h"
#include "debug.h"

extern uint32 boot_completed;

Condition::Condition(Mutex *lock) : sleepers_(), lock_(lock)
{
}

Condition::~Condition()
{
  if (sleepers_.size() != 0)
    kprintfd("WARNING: Condition::~Condition (%x) with sleepers_.size() != 0 and currentThread (%x)\n", this, currentThread);
}

void Condition::wait()
{
  if (likely(boot_completed))
  {
    // list is protected, because we assume, the lock is being held
    assert(lock_->isHeldBy(currentThread));
    assert(ArchInterrupts::testIFSet());
    sleepers_.push_back(currentThread);
    //<-- an interrupt and signal could happen here or during "sleep()"  ! problem: Thread* gets deleted before thread goes to sleep -> no wakeup call possible on next signal
    debug(CONDITION, "Condition::wait: Thread %x  %d:%s wating on Condition %x\n",currentThread,currentThread->getTID(),currentThread->getName(),this);
    Scheduler::instance()->sleepAndRelease(*lock_);
    assert(lock_);
    lock_->acquire();
  }
}

void Condition::waitWithoutReAcquire()
{
  if (likely(boot_completed))
  {
    // list is protected, because we assume, the lock is being held
    assert(lock_->isHeldBy(currentThread));
    assert(ArchInterrupts::testIFSet());
    sleepers_.push_back(currentThread);
    //<-- an interrupt and signal could happen here or during "sleep()"  ! problem: Thread* gets deleted before thread goes to sleep -> no wakeup call possible on next signal
    debug(CONDITION, "Condition::wait: Thread %x  %d:%s wating on Condition %x\n",currentThread,currentThread->getTID(),currentThread->getName(),this);
    Scheduler::instance()->sleepAndRelease(*lock_);
  }
}

void Condition::signal()
{
  if (likely(boot_completed))
  {
    if (! lock_->isHeldBy(currentThread))
      return;
    assert(ArchInterrupts::testIFSet());
    Thread *thread=0;
    if (!sleepers_.empty())
    {
      thread = sleepers_.front();
      if (thread->state_ == Sleeping)
      {
        //Solution to above Problem: Wake and Remove from List only Threads which are actually sleeping
        Scheduler::instance()->wake(thread);
        sleepers_.pop_front();
      }
    }
    if (thread)
      debug(CONDITION,"Condition::signal: Thread %x  %d:%s being signaled for Condition %x\n",thread,thread->getTID(),thread->getName(),this);
  }
}

void Condition::broadcast()
{
  if (likely(boot_completed))
  {
    if (! lock_->isHeldBy(currentThread))
      return;
    assert(ArchInterrupts::testIFSet());
    Thread *thread;
    ustl::list<Thread*> tmp_threads;
    while (!sleepers_.empty())
    {
      thread = sleepers_.front();
      sleepers_.pop_front();
      if (thread->state_ == Sleeping)
        Scheduler::instance()->wake(thread);
      else
      {
        assert(thread->state_ != Running && "Why is a *Running* thread on the sleepers list of this condition? bug?");
        tmp_threads.push_back(thread);
      }
      debug(CONDITION,"Condition::broadcast: Thread %x  %d:%s being signaled for Condition %x\n",thread,thread->getTID(),thread->getName(),this);
    }
    while (!tmp_threads.empty())
    {
      Thread* temp = tmp_threads.front();
      sleepers_.push_back(temp);
      tmp_threads.pop_front();
    }
  }
}
