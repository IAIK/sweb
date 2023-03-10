#include "Lock.h"
#include "kprintf.h"
#include "assert.h"
#include "Thread.h"
#include "ArchThreads.h"
#include "ArchInterrupts.h"
#include "Scheduler.h"
#include "Stabs2DebugInfo.h"
#include "SMP.h"
#include "ArchMulticore.h"
#include "SystemState.h"
extern Stabs2DebugInfo const* kernel_debug_info;

Lock::Lock(const char *name) :
  held_by_(nullptr),
  next_lock_on_holding_list_(nullptr),
  last_accessed_at_(0),
  name_(name ? name : ""),
  waiters_list_(nullptr),
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
    debug(LOCK, "ERROR: Lock::~Lock %s (%p): At least thread %p is still waiting on this lock,\n"
        "currentThread() is: %p, the thread holding this lock is: %p\n",
          name_, this, waiter, currentThread, held_by);
    lockWaitersList();
    printWaitersList();
    // The waiters list does not has to be unlocked here, because the kernel is going to die.
    assert(false && "A thread is still waiting on a lock that is about to be destroyed");
  }
  if(held_by)
  {
    debug(LOCK, "Warning: Lock::~Lock %s (%p): Thread %p is still holding this lock, currentThread() is: %p\n",
        name_, this, held_by, currentThread);
  }
}

void Lock::printWaitersList()
{
  debugAlways(LOCK, "Threads waiting for lock %s (%p), newest thread waiting first:\n", name_, this);
  size_t count = 0;
  for(Thread* thread = waiters_list_; thread != nullptr; thread = thread->next_thread_in_lock_waiters_list_)
  {
    debugAlways(LOCK, "%zu: %s (%p)\n", ++count, thread->getName(), thread);
  }
}

void Lock::printHoldingList(Thread* thread)
{
  debugAlways(LOCK, "Locks held by thread %s (%p):\n",
        thread->getName(), thread);
  for(Lock* lock = thread->holding_lock_list_; lock != nullptr; lock = lock->next_lock_on_holding_list_)
  {
      auto accessed_at = lock->last_accessed_at_;
      debugAlways(LOCK, " %s (%p), locked at %zx%s", lock->name_, lock, accessed_at, kernel_debug_info ? "" : "\n");
    if (kernel_debug_info)
        kernel_debug_info->printCallInformation(accessed_at);
  }
}

void Lock::pushFrontToCurrentThreadHoldingList()
{
  if(!currentThread)
    return;
  next_lock_on_holding_list_ = currentThread->holding_lock_list_;
  ArchThreads::atomic_set((pointer&)(currentThread->holding_lock_list_), (pointer)this);
}


void Lock::printStatus()
{
  Thread *thread = held_by_;
  pointer last_accessed_at = last_accessed_at_;
  if(!last_accessed_at && !thread)
  {
    debugAlways(LOCK, "Lock %s (%p) probably has not been used yet.\n", getName(), this);
    return;
  }

  if(kernel_debug_info)
  {
    if(thread)
    {
      debugAlways(LOCK, "Lock %s (%p) has been acquired by thread %s (%p) at: ",
            getName(), this, thread->getName(), thread);
    }
    else
    {
      debugAlways(LOCK, "Lock %s (%p) has been released at: ", getName(), this);
    }
    kernel_debug_info->printCallInformation(last_accessed_at);
  }
}

void Lock::checkForDeadLock()
{
  if(!currentThread)
    return;
  if(held_by_ == currentThread)
  {
    debug(LOCK, "Deadlock: Lock: %s (%p), held already by currentThread(): %s (%p).\n",
          name_, this, currentThread->getName(), currentThread);
    printStatus();
    assert(false && "Deadlock, lock already held by currentThread");
  }
  checkForCircularDeadLock(currentThread, this);
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
    for(current = currentThread->holding_lock_list_; current != nullptr; current = current->next_lock_on_holding_list_)
    {
      if(current->next_lock_on_holding_list_ == this)
      {
        ArchThreads::atomic_set((pointer&)(current->next_lock_on_holding_list_), (pointer)(this->next_lock_on_holding_list_));
        break;
      }
    }
  }
  next_lock_on_holding_list_ = nullptr;
}

void Lock::checkCurrentThreadStillWaitingOnAnotherLock()
{
  if(!currentThread)
    return;
  if(currentThread->lock_waiting_on_ != nullptr)
  {
    debug(LOCK, "ERROR: Lock: Thread %s (%p) is trying to lock %s (%p), eventhough is already waiting on lock %s (%p).\n"
          "You shouldn't use Scheduler::wake() with a thread sleeping on a lock!\n",
          currentThread->getName(), currentThread, name_, this,
          currentThread->lock_waiting_on_->getName(), currentThread->lock_waiting_on_);
    if(kernel_debug_info)
    {
      debug(LOCK, "The other lock has been acquired by ");
      kernel_debug_info->printCallInformation(currentThread->lock_waiting_on_->last_accessed_at_);
    }
    assert(false);
  }
}

void Lock::doChecksBeforeWaiting()
{
  assert(name_ && "name_ is null, maybe lock was not initialised?");
  // Check if the interrupts are set. Else, we maybe wait forever
  checkInterrupts("Lock::doChecksBeforeWaiting");
  // Check if the thread has been waken up even if he is already waiting on another lock.
  checkCurrentThreadStillWaitingOnAnotherLock();
  // Check if locking this lock would result in a deadlock.
  checkForDeadLock();
}


void Lock::checkForCircularDeadLock(Thread* thread_waiting, Lock* start)
{
  // Check all locks which are held by the target thread.
  // This can be done, because we know that the list may not be modified
  // (accept of an atomic push front, which does not matter at all)
  for(Lock* lock = thread_waiting->holding_lock_list_; lock != nullptr; lock = lock->next_lock_on_holding_list_)
  {
    // In case a thread which is indirectly waiting for the current thread
    // holds the lock, a deadlock happened.
    if(lock == start)
    {
      printOutCircularDeadLock(thread_waiting);
      assert(false && "Circular deadlock detected");
    }
    for(Thread* t_waiting_on_lock = lock->waiters_list_; t_waiting_on_lock != nullptr;
        t_waiting_on_lock = t_waiting_on_lock->next_thread_in_lock_waiters_list_)
    {
      // The method has to be called recursively, so it is possible to check indirect
      // deadlocks. The recursive approach may be slower than other checking methods,
      // but it is safe and more precise
      checkForCircularDeadLock(t_waiting_on_lock, start);
    }
  }
}

void Lock::printOutCircularDeadLock(Thread* starting)
{
  debugAlways(LOCK, "CIRCULAR DEADLOCK when waiting for %s (%p) with thread %s (%p)!\n",
        getName(), this, currentThread->getName(), currentThread);
  debugAlways(LOCK, "Printing out the circular deadlock:\n");
  currentThread->lock_waiting_on_ = this;
  // in this case we can access the other threads, because we KNOW that they are indirectly waiting on the current thread.
  for(Thread* thread = starting; thread != nullptr; thread = thread->lock_waiting_on_->held_by_)
  {
    Thread * holding = thread->lock_waiting_on_->held_by_;
    Lock* lock = thread->lock_waiting_on_;
    debugAlways(LOCK, "Thread %-40.40s (%p) holding lock %-40.40s (%p), waiting for lock %-40.40s (%p)\n",
          holding->getName(), holding, lock->getName(),
          lock, holding->lock_waiting_on_->getName(),
          holding->lock_waiting_on_);
    if(kernel_debug_info)
    {
      debugAlways(LOCK, "This lock has been locked at ");
      kernel_debug_info->printCallInformation(lock->last_accessed_at_);
    }
    // In the thread we are looking at is the current one, we have to stop.
    // It would result in an endless loop (circular print out ^^).
    if(thread == currentThread) break;
  }
}

void Lock::checkInterrupts(const char* method)
{
  // it would be nice to assert Scheduler::instance()->isSchedulingEnabled() as well.
  // unfortunately this is difficult because we might want to acquire/release locks
  // while scheduling is disabled
  if(unlikely((ArchInterrupts::testIFSet() == false) && (SMP::numRunningCpus() == 1)))
  {
    ArchInterrupts::disableInterrupts();
    debugAlways(LOCK, "(ERROR) %s: Lock %s (%p) with IF=0 and SchedulingEnabled=%d ! Now we're dead !!!\n"
          "Maybe you used new/delete in irq/int-Handler context or while Scheduling disabled?\n\n",
          method, name_, this, Scheduler::instance()->isSchedulingEnabled());
    assert(false && "Blocking on lock with disabled interrupts/scheduling");
  }
}

void Lock::lockWaitersList(bool yield)
{
  // The waiters list lock is a simple spinlock.
  // Just wait until the holding thread is releasing the lock,
  // and acquire it. These steps have to be atomic.
  while(ArchThreads::testSetLock(waiters_list_lock_, (size_t)1))
  {
    if(yield)
    {
        Scheduler::instance()->yield();
    }
  }
}

void Lock::unlockWaitersList()
{
  ArchThreads::syncLockRelease(waiters_list_lock_);
}

void Lock::pushFrontCurrentThreadToWaitersList()
{
  assert(currentThread);
  assert(waitersListIsLocked());
  currentThread->next_thread_in_lock_waiters_list_ = waiters_list_;
  // the following set has to be atomic
  // waiters_list_ = currentThread();
  ArchThreads::atomic_set((pointer&)(waiters_list_), (pointer)(currentThread));
}

Thread* Lock::popBackThreadFromWaitersList()
{
  assert(waitersListIsLocked());
  Thread* thread = nullptr;
  if(waiters_list_ == nullptr)
  {
    // the waiters list is empty
  }
  else if(waiters_list_->next_thread_in_lock_waiters_list_ == nullptr)
  {
    // this thread is the only one in the waiters list
    thread = waiters_list_;
    ArchThreads::atomic_set((pointer&)(waiters_list_), (pointer)(0));
    waiters_list_ = nullptr;
  }
  else
  {
    // there is more than one thread in the waiters list
    // search for the thread before the last thread in the list
    Thread* previos;
    for(previos = waiters_list_; previos->next_thread_in_lock_waiters_list_->next_thread_in_lock_waiters_list_ != nullptr;
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

    for(Thread* thread = waiters_list_; thread->next_thread_in_lock_waiters_list_ != nullptr;
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
}

void Lock::checkInvalidRelease(const char* method)
{
  assert(name_ && "name_ is null, maybe lock was not initialised?");
  if(unlikely(held_by_ != currentThread))
  {
    // push the information onto the stack, so the variable may not be modified meanwhile we are working with it
    Thread* holding = held_by_;

    debugAlways(LOCK, "%s: Lock %s (%p) currently not held by currentThread! "
          "Held by %s (%p), currentThread is %s (%p)\n", method, name_, this,
          (holding ? holding->getName() : "UNKNOWN THREAD"), holding, currentThread->getName(),
          currentThread);
    printStatus();
    if (currentThread)
    {
        debugAlways(LOCK, "Backtrace:\n");
        currentThread->printBacktrace(false);
    }

    assert(false && "Thread tried to release lock that it is not currently holding");
  }
}

void Lock::sleepAndRelease(bool should_yield)
{
  currentThread->lock_waiting_on_ = this;
  pushFrontCurrentThreadToWaitersList();
  {
      PreemptProtect p;
      // Ensure that other threads only see this thread in the waiters list
      // after it has already been set to sleep
      // Must not be interrupted between sleep and unlock
      Scheduler::instance()->sleep(false);
      unlockWaitersList();
  }
  if (should_yield)
  {
      Scheduler::instance()->yield();
  }
}
