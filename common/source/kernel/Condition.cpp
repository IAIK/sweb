#include "Condition.h"
#include "Thread.h"
#include "Mutex.h"
#include "Scheduler.h"
#include "assert.h"
#include "kprintf.h"
#include "debug.h"

Condition::Condition(Mutex* mutex, const char* name) :
  Lock(name), mutex_(mutex)
{
}

void Condition::wait(const char* debug_info, bool re_acquire_mutex)
{
  if(unlikely(system_state != RUNNING))
    return;
  //debug(LOCK, "Condition::wait: Thread %s (%p) waiting on condition %s (%p).\n",
  //      currentThread->getName(), currentThread, getName(), this);
  assert(mutex_->isHeldBy(currentThread));
  // check if the interrupts are enabled and the thread is not waiting for some other locks
  checkInterrupts("Condition::wait", debug_info);
  checkCurrentThreadStillWaitingOnAnotherLock(debug_info);

  assert(currentThread->holding_lock_list_);
  if(currentThread->holding_lock_list_->hasNextOnHoldingList())
  {
    debug(LOCK, "Condition::wait: Warning: The thread %s (%p) is holding some locks when going to sleep on condition %s (%p).\n"
          "This may lower the performance of the system, and cause undetectable deadlocks!\n",
          currentThread->getName(), currentThread, getName(), this);
    printHoldingList(currentThread);
  }
  lockWaitersList();
  // The mutex can be released here, because for waking up another thread, the list lock is needed, which is still held by the thread.
  mutex_->release();
  Scheduler::instance()->sleepAndRelease(*(Lock*)this);
  if(re_acquire_mutex)
  {
    assert(mutex_);
    mutex_->acquire();
  }
}

void Condition::signal(const char* debug_info)
{
  if(unlikely(system_state != RUNNING))
    return;
  assert(mutex_->isHeldBy(currentThread));
  checkInterrupts("Condition::signal", debug_info);
  lockWaitersList();
  Thread* thread_to_be_woken_up = popBackThreadFromWaitersList();
  unlockWaitersList();

  if(thread_to_be_woken_up)
  {
    if(likely(thread_to_be_woken_up->state_ == Sleeping))
    {
      // In this case we can access the pointer of the other thread without locking,
      // because we can ensure that the thread is sleeping.

      //debug(LOCK, "Condition: Thread %s (%p) being signaled for condition %s (%p).\n",
      //      thread_to_be_woken_up->getName(), thread_to_be_woken_up, getName(), this);
      thread_to_be_woken_up->lock_waiting_on_ = 0;
      Scheduler::instance()->wake(thread_to_be_woken_up);
    }
    else
    {
      debug(LOCK, "ERROR: Condition %s (%p): Thread %s (%p) is in state %s AND waiting on the condition!\n",
            getName(), this, thread_to_be_woken_up->getName(), thread_to_be_woken_up,
            Thread::threadStatePrintable[thread_to_be_woken_up->state_]);
      if(debug_info) debug(LOCK, "Debug Info: %s\n", debug_info);
      assert(false);
    }
  }
}

void Condition::broadcast(const char* debug_info)
{
  if(unlikely(system_state != RUNNING))
    return;
  assert(mutex_->isHeldBy(currentThread));
  // signal em all
  while(threadsAreOnWaitersList())
  {
    signal(debug_info);
  }
}
