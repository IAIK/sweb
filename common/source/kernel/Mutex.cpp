#include "Mutex.h"
#include "kprintf.h"
#include "ArchThreads.h"
#include "ArchInterrupts.h"
#include "Scheduler.h"
#include "Thread.h"
#include "panic.h"

Mutex::Mutex(const char* name) :
  Lock::Lock(name), mutex_(0)
{
}

bool Mutex::acquireNonBlocking(const char* debug_info)
{
  if(unlikely(system_state != RUNNING))
    return true;
  //debug(LOCK, "Mutex::acquireNonBlocking:  Mutex: %s (%p), currentThread: %s (%p).\n",
  //         getName(), this, currentThread->getName(), currentThread);

  // There may be some cases where the pre-checks may not be wished here.
  // But these cases are usually dirty implemented, and it would not be necessary to call this method there.
  // So in case you see this comment, re-think your implementation and don't just comment out this line!
  doChecksBeforeWaiting(debug_info);

  if(ArchThreads::testSetLock(mutex_, 1))
  {
    // The mutex is already held by another thread,
    // so we are not allowed to lock it.
    return false;
  }
  assert(held_by_ == 0);
  held_by_ = currentThread;
  pushFrontToCurrentThreadHoldingList();
  return true;
}

void Mutex::acquire(const char* debug_info)
{
  if(unlikely(system_state != RUNNING))
    return;
  //debug(LOCK, "Mutex::acquire:  Mutex: %s (%p), currentThread: %s (%p).\n",
  //         getName(), this, currentThread->getName(), currentThread);
  while(ArchThreads::testSetLock(mutex_, 1))
  {
    checkCurrentThreadStillWaitingOnAnotherLock(debug_info);
    lockWaitersList();
    // Here we have to check for the lock again, in case some one released it in between, we might sleep forever.
    if(!ArchThreads::testSetLock(mutex_, 1))
    {
      unlockWaitersList();
      break;
    }
    // check for deadlocks, interrupts...
    doChecksBeforeWaiting(debug_info);
    Scheduler::instance()->sleepAndRelease(*(Lock*)this);
    // We have been waken up again.
    currentThread->lock_waiting_on_ = 0;
  }

  assert(held_by_ == 0);
  pushFrontToCurrentThreadHoldingList();
  held_by_ = currentThread;
}

void Mutex::release(const char* debug_info)
{
  if(unlikely(system_state != RUNNING))
    return;
  //debug(LOCK, "Mutex::release:  Mutex: %s (%p), currentThread: %s (%p).\n",
  //         getName(), this, currentThread->getName(), currentThread);
  checkInvalidRelease("Mutex::release", debug_info);
  removeFromCurrentThreadHoldingList();
  held_by_ = 0;
  mutex_ = 0;
  // Wake up a sleeping thread. It is okay that the mutex is not held by the current thread any longer.
  // In worst case a new thread is woken up. Otherwise (first wake up, then release),
  // it could happen that a thread is going to sleep after the this one is trying to wake up one.
  // Then we are dead... (the thread may sleep forever, in case no other thread is going to acquire this mutex again).
  lockWaitersList();
  Thread* thread_to_be_woken_up = popBackThreadFromWaitersList();
  unlockWaitersList();
  if(thread_to_be_woken_up)
  {
    Scheduler::instance()->wake(thread_to_be_woken_up);
  }
}

bool Mutex::isFree()
{
  if(unlikely(ArchInterrupts::testIFSet() && Scheduler::instance()->isSchedulingEnabled()))
  {
    debug(LOCK, "Mutex::isFree: ERROR: Should not be used with IF=1 AND enabled Scheduler, use acquire instead\n");
    assert(false);
  }
  return (mutex_ == 0);
}
