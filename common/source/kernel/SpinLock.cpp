#include "SpinLock.h"
#include "kprintf.h"
#include "ArchThreads.h"
#include "ArchInterrupts.h"
#include "panic.h"
#include "Scheduler.h"
#include "Thread.h"

SpinLock::SpinLock(const char* name) :
  Lock::Lock(name), lock_(0)
{
}

bool SpinLock::acquireNonBlocking(const char* debug_info)
{
  if(unlikely(system_state != RUNNING))
    return true;
  // There may be some cases where the pre-checks may not be wished here.
  // But these cases are usually dirty implemented, and it would not be necessary to call this method there.
  // So in case you see this comment, re-think your implementation and don't just comment out this line!
  doChecksBeforeWaiting(debug_info);

  if(ArchThreads::testSetLock(lock_, 1))
  {
    // The spinlock is held by another thread at the moment
    return false;
  }
  // The spinlock is now held by the current thread.
  assert(held_by_ == 0);
  held_by_ = currentThread;
  pushFrontToCurrentThreadHoldingList();
  return true;
}

void SpinLock::acquire(const char* debug_info)
{
  if(unlikely(system_state != RUNNING))
    return;
  //  debug(LOCK, "Spinlock::acquire: Acquire spinlock %s (%p) with thread %s (%p)\n",
  //        getName(), this, currentThread->getName(), currentThread);
  if(ArchThreads::testSetLock(lock_, 1))
  {
    // We did not directly managed to acquire the spinlock, need to check for deadlocks and
    // to push the current thread to the waiters list.
    doChecksBeforeWaiting(debug_info);

    currentThread->lock_waiting_on_ = this;
    lockWaitersList();
    pushFrontCurrentThreadToWaitersList();
    unlockWaitersList();

    // here comes the basic spinlock
    while(ArchThreads::testSetLock(lock_, 1))
    {
      //SpinLock: Simplest of Locks, do the next best thing to busy waiting
      Scheduler::instance()->yield();
    }
    // Now we managed to acquire the spinlock. Remove the current thread from the waiters list.
    lockWaitersList();
    removeCurrentThreadFromWaitersList();
    unlockWaitersList();
    currentThread->lock_waiting_on_ = 0;
  }
  // The current thread is now holding the spinlock
  held_by_ = currentThread;
  pushFrontToCurrentThreadHoldingList();
}

bool SpinLock::isFree()
{
  if(unlikely(ArchInterrupts::testIFSet() && Scheduler::instance()->isSchedulingEnabled()))
  {
    debug(LOCK, "SpinLock::isFree: ERROR: Should not be used with IF=1 AND enabled Scheduler, use acquire instead\n");
    assert(false);
  }
  return (lock_ == 0);
}

void SpinLock::release(const char* debug_info)
{
  if(unlikely(system_state != RUNNING))
    return;
  //debug(LOCK, "Spinlock::release: Release spinlock %s (%p) with thread %s (%p)\n",
  //      getName(), this, currentThread->getName(), currentThread);
  checkInvalidRelease("SpinLock::release", debug_info);
  removeFromCurrentThreadHoldingList();
  held_by_ = 0;
  lock_ = 0;
}

