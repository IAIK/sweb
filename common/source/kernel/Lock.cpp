#include "Lock.h"
#include "kprintf.h"
#include "assert.h"
#include "Thread.h"
#include "ArchThreads.h"
#include "ArchInterrupts.h"
#include "Scheduler.h"

Lock::Lock(const char *name) :
  held_by_(0),
  next_lock_on_holding_list_(0),
  name_(name ? name : ""),
  waiters_list_(0),
  waiters_list_lock_(0)
{
}

Lock::~Lock()
{
  if(unlikely(system_state != RUNNING))
    return;
  // copy the pointers to the stack because it may be reseted before printing the element out.
  Thread* waiter = waiters_list_;
  Thread* held_by = held_by_;
  if(waiter)
  {
    debug(LOCK, "ERROR: Lock::~Lock %s (%x): At least thread %x is still waiting on this lock,\n"
        "currentThread is: %x, the thread holding this lock is: %x\n",
        name_, this, waiter, currentThread, held_by);
    lockWaitersList();
    printWaitersList();
    // The waiters list does not has to be unlocked here, because the kernel is going to die.
    assert(false);
  }
  if(held_by)
  {
    debug(LOCK, "Warning: Lock::~Lock %s (%x): Thread %x is still holding this lock, currentThread is: %x\n",
        name_, this, held_by, currentThread);
  }
}

void Lock::printWaitersList()
{
  debug(LOCK, "Threads waiting for lock %s (%p), newest thread waiting first:\n", name_, this);
  size_t count = 0;
  for(Thread* thread = waiters_list_; thread != 0; thread = thread->next_thread_in_lock_waiters_list_)
  {
    kprintfd("%u: %s (%x)\n", ++count, thread->getName(), thread);
  }
}

void Lock::printHoldingList(Thread* thread)
{
  debug(LOCK, "Locks held by thread %s (%p):",
        thread->getName(), thread);
  for(Lock* lock = thread->holding_lock_list_; lock != 0; lock = lock->next_lock_on_holding_list_)
  {
    kprintfd(" %s (%p)%s", lock->name_, lock, lock->next_lock_on_holding_list_ ? "," : ".\n");
  }
}

void Lock::pushFrontToCurrentThreadHoldingList()
{
  if(!currentThread)
    return;
  next_lock_on_holding_list_ = currentThread->holding_lock_list_;
  ArchThreads::atomic_set((pointer&)(currentThread->holding_lock_list_), (pointer)this);
}

void Lock::checkForDeadLock(const char* debug_info)
{
  if(!currentThread)
    return;
  if(held_by_ == currentThread)
  {
    debug(LOCK, "Deadlock: Lock: %s (%p), held already by currentThread: %s (%p).\n",
          name_, this, currentThread->getName(), currentThread);
    if(debug_info) debug(LOCK, "Debug Info: %s\n", debug_info);

    assert(false);
  }
  checkForCircularDeadLock(currentThread, this, debug_info);
}

void Lock::removeFromCurrentThreadHoldingList()
{
  if(!currentThread)
    return;
  if(currentThread->holding_lock_list_ == this)
  {
    ArchThreads::atomic_set((pointer&)(currentThread->holding_lock_list_), (pointer)(this->next_lock_on_holding_list_));
  }
  else
  {
    Lock* current;
    for(current = currentThread->holding_lock_list_; current != 0; current = current->next_lock_on_holding_list_)
    {
      if(current->next_lock_on_holding_list_ == this)
      {
        ArchThreads::atomic_set((pointer&)(current->next_lock_on_holding_list_), (pointer)(this->next_lock_on_holding_list_));
        break;
      }
    }
  }
  next_lock_on_holding_list_ = 0;
  return;
}

void Lock::checkCurrentThreadStillWaitingOnAnotherLock(const char* debug_info)
{
  if(!currentThread)
    return;
  if(currentThread->lock_waiting_on_ != 0)
  {
    debug(LOCK, "ERROR: Lock: Thread %s (%p) is already waiting on lock %s (%p).\n"
          "You shouldn't use Scheduler::wake() with a thread sleeping on a lock!\n",
          currentThread->getName(), currentThread, name_, this);
    if(debug_info) debug(LOCK, "Debug Info: %s\n", debug_info);
    assert(false);
  }
}

void Lock::doChecksBeforeWaiting(const char* debug_info)
{
  // Check if the interrupts are set. Else, we maybe wait forever
  checkInterrupts("Lock::doPreChecks", debug_info);
  // Check if the thread has been waken up even if he is already waiting on another lock.
  checkCurrentThreadStillWaitingOnAnotherLock(debug_info);
  // Check if locking this lock would result in a deadlock.
  checkForDeadLock(debug_info);
}

void Lock::checkForCircularDeadLock(Thread* thread_waiting, Lock* start, const char* debug_info)
{
  // Check all locks which are held by the target thread.
  // This can be done, because we know that the list may not be modified
  // (accept of an atomic push front, which does not matter at all)
  for(Lock* lock = thread_waiting->holding_lock_list_; lock != 0; lock = lock->next_lock_on_holding_list_)
  {
    // In case a thread which is indirectly waiting for the current thread
    // holds the lock, a deadlock happened.
    if(lock == start)
    {
      printOutCircularDeadLock(thread_waiting);
      if(debug_info) debug(LOCK, "Debug Info: %s\n", debug_info);
      assert(false);
    }
    for(Thread* thread_waiting = lock->waiters_list_; thread_waiting != 0;
        thread_waiting = thread_waiting->next_thread_in_lock_waiters_list_)
    {
      // The method has to be called recursively, so it is possible to check indirect
      // deadlocks. The recursive approach may be slower than other checking methods,
      // but it is safe and more precise
      checkForCircularDeadLock(thread_waiting, start, debug_info);
    }
  }
}

void Lock::printOutCircularDeadLock(Thread* starting)
{
  debug(LOCK, "CIRCULAR DEADLOCK when waiting for %s (%p) with thread %s (%x)!\n"
        "Printing out the circular deadlock:\n",
        getName(), this, currentThread->getName(), currentThread);
  currentThread->lock_waiting_on_ = this;
  // in this case we can access the other threads, because we KNOW that they are indirectly waiting on the current thread.
  for(Thread* thread = starting; thread != 0; thread = thread->lock_waiting_on_->held_by_)
  {
    debug(LOCK, "Thread %s (%x) holding lock %s (%p), waiting for lock %s (%x)\n",
          thread->getName(), thread, thread->lock_waiting_on_->getName(),
          thread->lock_waiting_on_, thread->lock_waiting_on_->held_by_->lock_waiting_on_->getName(),
          thread->lock_waiting_on_->held_by_->lock_waiting_on_);
    // In the thread we are looking at is the current one, we have to stop.
    // It would result in an endless loop (circular print out ^^).
    if(thread == currentThread) break;
  }
}

void Lock::checkInterrupts(const char* method, const char* debug_info)
{
  // it would be nice to assert Scheduler::instance()->isSchedulingEnabled() as well.
  // unfortunately this is difficult because we might want to acquire/release locks
  // while scheduling is disabled
  if(unlikely(ArchInterrupts::testIFSet() == false))
  {
    ArchInterrupts::disableInterrupts();
    debug(LOCK, "(ERROR) %s: Lock %s (%p) with IF=%d and SchedulingEnabled=%d ! Now we're dead !!!\n"
          "Maybe you used new/delete in irq/int-Handler context or while Scheduling disabled?\n\n",
          method, name_, this, ArchInterrupts::testIFSet(), Scheduler::instance()->isSchedulingEnabled());
    if(debug_info) debug(LOCK, "Debug Info: %s\n", debug_info);
    assert(false);
  }
}

void Lock::lockWaitersList()
{
  // The waiters list lock is a simple spinlock.
  // Just wait until the holding thread is releasing the lock,
  // and acquire it. These steps have to be atomic.
  while(ArchThreads::testSetLock(waiters_list_lock_, 1))
  {
    Scheduler::instance()->yield();
  }
}

void Lock::unlockWaitersList()
{
  waiters_list_lock_ = 0;
}

void Lock::pushFrontCurrentThreadToWaitersList()
{
  assert(currentThread);
  assert(waitersListIsLocked());
  currentThread->next_thread_in_lock_waiters_list_ = waiters_list_;
  // the following set has to be atomic
  // waiters_list_ = currentThread;
  ArchThreads::atomic_set((pointer&)(waiters_list_), (pointer)(currentThread));
}

Thread* Lock::popBackThreadFromWaitersList()
{
  assert(waitersListIsLocked());
  Thread* thread = 0;
  if(waiters_list_ == 0)
  {
    // the waiters list is empty
  }
  else if(waiters_list_->next_thread_in_lock_waiters_list_ == 0)
  {
    // this thread is the only one in the waiters list
    thread = waiters_list_;
    ArchThreads::atomic_set((pointer&)(waiters_list_), (pointer)(0));
    waiters_list_ = 0;
  }
  else
  {
    // there is more than one thread in the waiters list
    // search for the thread before the last thread in the list
    Thread* previos;
    for(previos = waiters_list_; previos->next_thread_in_lock_waiters_list_->next_thread_in_lock_waiters_list_ != 0;
        previos = previos->next_thread_in_lock_waiters_list_);

    thread = previos->next_thread_in_lock_waiters_list_;
    // remove the last element
    ArchThreads::atomic_set((pointer&)(previos->next_thread_in_lock_waiters_list_), (pointer)(0));
  }
  return thread;
}

void Lock::removeCurrentThreadFromWaitersList()
{
  if(!currentThread)
    return;
  assert(waitersListIsLocked());
  assert(waiters_list_);
  if(currentThread == waiters_list_)
  {
    // the current thread is the first element
    ArchThreads::atomic_set((pointer&)(waiters_list_), (pointer)(currentThread->next_thread_in_lock_waiters_list_));
  }
  else
  {

    for(Thread* thread = waiters_list_; thread->next_thread_in_lock_waiters_list_ != 0;
        thread = thread->next_thread_in_lock_waiters_list_)
    {
      if(thread->next_thread_in_lock_waiters_list_ == currentThread)
      {

        ArchThreads::atomic_set((pointer&)(thread->next_thread_in_lock_waiters_list_),
                                (pointer)(currentThread->next_thread_in_lock_waiters_list_));
        break;
      }
    }
  }
  ArchThreads::atomic_set((pointer&)(currentThread->next_thread_in_lock_waiters_list_ ), (pointer)0);
  return;
}

void Lock::checkInvalidRelease(const char* method, const char* debug_info)
{
  if(unlikely(held_by_ != currentThread))
  {
    // push held by onto the stack, so the variable may not be modified meanwhile we are printing the name out
    Thread* holding = held_by_;

    debug(LOCK, "%s: Lock %s (%p) currently not held by currentThread!\n"
          "held_by %s (%p), currentThread %s (%p)\n", method, name_, this,
          (holding ? holding->getName() : "(0)"), holding, currentThread->getName(),
          currentThread);
    if(debug_info) debug(LOCK, "Debug Info: %s\n", debug_info);

    assert(false);
  }
}
