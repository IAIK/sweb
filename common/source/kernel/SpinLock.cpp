#include "SpinLock.h"
#include "kprintf.h"
#include "SMP.h"
#include "ArchThreads.h"
#include "ArchInterrupts.h"
#include "ArchMulticore.h"
#include "assert.h"
#include "Scheduler.h"
#include "Thread.h"
#include "assert.h"
#include "Stabs2DebugInfo.h"
#include "backtrace.h"
#include "SystemState.h"
extern Stabs2DebugInfo const *kernel_debug_info;

SpinLock::SpinLock(const char* name) :
  Lock::Lock(name), lock_(0)
{
}

bool SpinLock::acquireNonBlocking(pointer called_by)
{
  if(unlikely(system_state != RUNNING))
    return true;
  if(!called_by)
    called_by = getCalledBefore(1);
//  debug(LOCK, "Spinlock::acquireNonBlocking: Acquire spinlock %s (%p) with thread %s (%p)\n",
//        getName(), this, currentThread()->getName(), currentThread());
//  if(kernel_debug_info)
//  {
//    debug(LOCK, "The acquire is called by: ");
//    kernel_debug_info->printCallInformation(called_by);
//  }

  // There may be some cases where the pre-checks may not be wished here.
  // But these cases are usually dirty implemented, and it would not be necessary to call this method there.
  // So in case you see this comment, re-think your implementation and don't just comment out this line!
  doChecksBeforeWaiting();

  if(ArchThreads::testSetLock(lock_, (size_t)1))
  {
    // The spinlock is held by another thread at the moment
    return false;
  }
  // The spinlock is now held by the current thread.
  assert(held_by_ == nullptr);
  last_accessed_at_ = called_by;
  held_by_ = currentThread;
  pushFrontToCurrentThreadHoldingList();
  return true;
}

void SpinLock::acquire(pointer called_by, bool yield)
{
  if(!called_by)
    called_by = getCalledBefore(1);

  if(system_state == RUNNING)
  {
    debug(LOCK, "CPU %zx acquiring spinlock %s (%p), called by: %zx\n", SMP::currentCpuId(), getName(), this, called_by);
  }
  else
  {
    debug(LOCK, "acquiring spinlock %s (%p), called by: %zx\n", getName(), this, called_by);
  }

//  debug(LOCK, "Spinlock::acquire: Acquire spinlock %s (%p) with thread %s (%p)\n",
//        getName(), this, currentThread()->getName(), currentThread());
//  if(kernel_debug_info)
//  {
//    debug(LOCK, "The acquire is called by: ");
//    kernel_debug_info->printCallInformation(called_by);
//  }
  if(ArchThreads::testSetLock(lock_, (size_t)1))
  {
    debug(LOCK, "didn't get spinlock %s (%p), called by: %zx\n", getName(), this, called_by);
    // We did not directly managed to acquire the spinlock, need to check for deadlocks and
    // to push the current thread to the waiters list.
    doChecksBeforeWaiting();

    //assert(currentThread); // debug
    if(currentThread)
    {
      currentThread->lock_waiting_on_ = this;
      lockWaitersList(yield);
      pushFrontCurrentThreadToWaitersList();
      unlockWaitersList();
    }

    // here comes the basic spinlock
    while(ArchThreads::testSetLock(lock_, (size_t)1))
    {
        ArchCommon::spinlockPause();
      //SpinLock: Simplest of Locks, do the next best thing to busy waiting
      if(currentThread && yield)
      {
        ArchInterrupts::yieldIfIFSet();
      }
    }
    // Now we managed to acquire the spinlock. Remove the current thread from the waiters list.
    if(currentThread)
    {
      lockWaitersList(yield);
      removeCurrentThreadFromWaitersList();
      unlockWaitersList();
      currentThread->lock_waiting_on_ = nullptr;
    }
  }
  debug(LOCK, "got spinlock %s (%p), called by: %zx\n", getName(), this, called_by);
  // The current thread is now holding the spinlock
  last_accessed_at_ = called_by;
  held_by_ = currentThread;
  pushFrontToCurrentThreadHoldingList();
}

bool SpinLock::isFree() const
{
  if(unlikely(ArchInterrupts::testIFSet() && Scheduler::instance()->isSchedulingEnabled() && !(SMP::numRunningCpus() > 1)))
  {
    return false;
    //debug(LOCK, "SpinLock::isFree: ERROR: Should not be used with IF=1 AND enabled Scheduler, use acquire instead\n");
    //assert(false);
  }
  return (lock_ == 0);
}

void SpinLock::release(pointer called_by)
{
  if(!called_by)
    called_by = getCalledBefore(1);

  if(system_state == RUNNING)
  {
          debug(LOCK, "CPU %zx releasing spinlock %s (%p), called by: %zx\n", SMP::currentCpuId(), getName(), this, called_by);
  }
  else
  {
          debug(LOCK, "releasing spinlock %s (%p), called by: %zx\n", getName(), this, called_by);
  }

//  debug(LOCK, "Spinlock::release: Release spinlock %s (%p) with thread %s (%p)\n",
//        getName(), this, currentThread()->getName(), currentThread());
//  if(kernel_debug_info)
//  {
//    debug(LOCK, "The release is called by: ");
//    kernel_debug_info->printCallInformation(called_by);
//  }
  checkInvalidRelease("SpinLock::release");
  removeFromCurrentThreadHoldingList();
  last_accessed_at_ = called_by;
  held_by_ = nullptr;
  lock_ = 0;
}
