#include "Condition.h"
#include "Thread.h"
#include "Mutex.h"
#include "Scheduler.h"
#include "assert.h"
#include "kprintf.h"
#include "debug.h"
#include "Stabs2DebugInfo.h"
#include "backtrace.h"

Condition::Condition(Mutex* mutex, const char* name) :
  Lock(name), mutex_(mutex)
{
}

void Condition::waitAndRelease(pointer called_by)
{
  if(!called_by)
    called_by = getCalledBefore(1);
  wait(false, called_by);
}

void Condition::wait(bool re_acquire_mutex, pointer called_by)
{
  if(unlikely(system_state != RUNNING))
    return;
  if(!called_by)
    called_by = getCalledBefore(1);
//  debug(LOCK, "Condition::wait: Thread %s (%p) is waiting on condition %s (%p).\n",
//        currentThread->getName(), currentThread, getName(), this);
//  if(kernel_debug_info)
//  {
//    debug(LOCK, "The wait is called by: ");
//    kernel_debug_info->printCallInformation(called_by);
//  }

  assert(mutex_->isHeldBy(currentThread));
  // check if the interrupts are enabled and the thread is not waiting for some other locks
  checkInterrupts("Condition::wait");
  checkCurrentThreadStillWaitingOnAnotherLock();

  assert(currentThread->holding_lock_list_);
  if(currentThread->holding_lock_list_->hasNextOnHoldingList())
  {
    debug(LOCK, "Condition::wait: Warning: The thread %s (%p) is holding some locks when going to sleep on condition %s (%p).\n"
          "This may lower the performance of the system, and cause undetectable deadlocks!\n",
          currentThread->getName(), currentThread, getName(), this);
    printHoldingList(currentThread);
  }
  lockWaitersList();
  last_accessed_at_ = called_by;
  // The mutex can be released here, because for waking up another thread, the list lock is needed, which is still held by the thread.
  mutex_->release(called_by);
  Scheduler::instance()->sleepAndRelease(*(Lock*)this);
  if(re_acquire_mutex)
  {
    assert(mutex_);
    mutex_->acquire(called_by);
  }
}

void Condition::signal(pointer called_by, bool locked_waiters_list)
{
  if(unlikely(system_state != RUNNING))
    return;
  if(!called_by)
    called_by = getCalledBefore(1);
//  debug(LOCK, "Condition::signal: Thread %s (%p) is signaling condition %s (%p).\n",
//        currentThread->getName(), currentThread, getName(), this);
//  debug(LOCK, "The signal is called by: ");
//  kernel_debug_info->printCallInformation(called_by);

  assert(mutex_->isHeldBy(currentThread));
  checkInterrupts("Condition::signal");
  if(!locked_waiters_list)
    lockWaitersList();
  last_accessed_at_ = called_by;
  Thread* thread_to_be_woken_up = popBackThreadFromWaitersList();
  if(!locked_waiters_list)
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
      assert(false);
    }
  }
}

void Condition::broadcast(pointer called_by)
{
  if(unlikely(system_state != RUNNING))
    return;
  if(!called_by)
    called_by = getCalledBefore(1);
  assert(mutex_->isHeldBy(currentThread));
  // signal em all
  lockWaitersList();
  while(threadsAreOnWaitersList())
  {
    signal(called_by, true);
  }
  unlockWaitersList();
}
