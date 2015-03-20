#include "Scheduler.h"
#include "Thread.h"
#include "panic.h"
#include "ArchThreads.h"
#include "ArchCommon.h"
#include "kprintf.h"
#include "ArchInterrupts.h"
#include "KernelMemoryManager.h"
#include <ulist.h>
#include "backtrace.h"
#include "ArchThreads.h"
#include "Mutex.h"
#include "umap.h"
#include "ustring.h"
#include "Lock.h"

ArchThreadInfo *currentThreadInfo;
Thread *currentThread;

Scheduler *Scheduler::instance_ = 0;

Scheduler *Scheduler::instance()
{
  if (unlikely(!instance_))
    instance_ = new Scheduler();
  return instance_;
}

Scheduler::Scheduler()
{
  block_scheduling_ = 0;
  ticks_ = 0;
  addNewThread(&cleanup_thread_);
  addNewThread(&idle_thread_);
}

uint32 Scheduler::schedule()
{
  if (block_scheduling_ != 0)
  {
    debug(SCHEDULER, "schedule: currently blocked\n");
    return 0;
  }

  Thread* previousThread = currentThread;
  do
  {
    currentThread = threads_.front();

    ustl::rotate(threads_.begin(), threads_.begin() + 1, threads_.end()); // no new/delete here - important because interrupts are disabled

    if ((currentThread == previousThread) && (currentThread->state_ != Running))
    {
      debug(SCHEDULER, "Scheduler::schedule: ERROR: currentThread == previousThread! Either no thread is in state Running or you added the same thread more than once.");
    }
  } while (!currentThread->schedulable());
//  debug ( SCHEDULER,"Scheduler::schedule: new currentThread is %x %s, switch_userspace:%d\n",currentThread,currentThread ? currentThread->getName() : 0,currentThread ? currentThread->switch_to_userspace_ : 0);

  uint32 ret = 1;

  if (currentThread->switch_to_userspace_)
    currentThreadInfo = currentThread->user_arch_thread_info_;
  else
  {
    currentThreadInfo = currentThread->kernel_arch_thread_info_;
    ret = 0;
  }

  return ret;
}

void Scheduler::addNewThread(Thread *thread)
{
  debug(SCHEDULER, "addNewThread: %x  %d:%s\n", thread, thread->getTID(), thread->getName());
  KernelMemoryManager::instance()->getKMMLock().acquire("in addNewThread");
  lockScheduling();
  KernelMemoryManager::instance()->getKMMLock().release("in addNewThread");
  threads_.push_back(thread);
  unlockScheduling();
}

void Scheduler::invokeCleanup()
{
  cleanup_thread_.addJob();
}

void Scheduler::sleep()
{
  currentThread->state_ = Sleeping;
  assert(block_scheduling_ == 0);
  yield();
}

void Scheduler::wake(Thread* thread_to_wake)
{
  thread_to_wake->state_ = thread_to_wake->isWorker() ? Worker : Running;
}

void Scheduler::yield()
{
  assert(this);
  if (!ArchInterrupts::testIFSet())
  {
    assert(currentThread);
    kprintfd("Scheduler::yield: WARNING Interrupts disabled, do you really want to yield ? (currentThread %x %s)\n",
             currentThread, currentThread->name_.c_str());
    currentThread->printBacktrace();
  }
  ArchThreads::yield();
}

void Scheduler::cleanupDeadThreads()
{
  lockScheduling();
  uint32 thread_count_max = threads_.size();
  if (thread_count_max > 1024)
    thread_count_max = 1024;
  Thread* destroy_list[thread_count_max];
  uint32 thread_count = 0;
  for (uint32 i = 0; i < threads_.size(); ++i)
  {
    Thread* tmp = threads_[i];
    if (tmp->state_ == ToBeDestroyed)
    {
      destroy_list[thread_count++] = tmp;
      threads_.erase(threads_.begin() + i); // Note: erase will not realloc!
      --i;
    }
    if (thread_count >= thread_count_max)
      break;
  }
  unlockScheduling();
  if (thread_count > 0)
  {
    for (uint32 i = 0; i < thread_count; ++i)
    {
      delete destroy_list[i];
      cleanup_thread_.jobDone();
    }
    debug(SCHEDULER, "cleanupDeadThreads: done\n");
  }
}

void Scheduler::printThreadList()
{
  uint32 c = 0;
  lockScheduling();
  debug(SCHEDULER, "Scheduler::printThreadList: %d Threads in List\n", threads_.size());
  for (c = 0; c < threads_.size(); ++c)
    debug(SCHEDULER, "Scheduler::printThreadList: threads_[%d]: %x  %d:%s     [%s]\n", c, threads_[c],
          threads_[c]->getTID(), threads_[c]->getName(), Thread::threadStatePrintable[threads_[c]->state_]);
  unlockScheduling();
}

void Scheduler::lockScheduling() //not as severe as stopping Interrupts
{
  if (unlikely(ArchThreads::testSetLock(block_scheduling_, 1)))
    kpanict((uint8*) "FATAL ERROR: Scheduler::*: block_scheduling_ was set !! How the Hell did the program flow get here then ?\n");
}

void Scheduler::unlockScheduling()
{
  block_scheduling_ = 0;
}

bool Scheduler::isSchedulingEnabled()
{
  if (this)
    return (block_scheduling_ == 0);
  else
    return false;
}

bool Scheduler::isCurrentlyCleaningUp()
{
  return currentThread == &cleanup_thread_;
}

uint32 Scheduler::getTicks()
{
  return ticks_;
}

void Scheduler::incTicks()
{
  ++ticks_;
}

void Scheduler::printStackTraces()
{
  lockScheduling();
  debug(BACKTRACE, "printing the backtraces of <%d> threads:\n", threads_.size());

  for (ustl::list<Thread*>::iterator it = threads_.begin(); it != threads_.end(); ++it)
  {
    (*it)->printBacktrace();
    debug(BACKTRACE, "\n");
    debug(BACKTRACE, "\n");
  }

  unlockScheduling();
}

static void printUserSpaceTracesHelper()
{
  currentThread->printUserBacktrace();
  currentThread->switch_to_userspace_ = 1;
  Scheduler::instance()->yield();
}

void Scheduler::printLockingInformation()
{
  size_t thread_count;
  Thread* thread;
  lockScheduling();
  kprintfd("\n");
  debug(LOCK, "Scheduler::printLockingInformation:\n");
  for (thread_count = 0; thread_count < threads_.size(); ++thread_count)
  {
    thread = threads_[thread_count];
    if(thread->holding_lock_list_ != 0)
    {
      Lock::printHoldingList(threads_[thread_count]);
    }
  }
  for (thread_count = 0; thread_count < threads_.size(); ++thread_count)
  {
    thread = threads_[thread_count];
    if(thread->lock_waiting_on_ != 0)
    {
      debug(LOCK, "Thread %s (0x%x) is waiting on lock: %s (0x%x).\n", thread->getName(), thread,
            thread->lock_waiting_on_ ->getName(), thread->lock_waiting_on_ );
    }
  }
  debug(LOCK, "Scheduler::printLockingInformation finished\n");
  unlockScheduling();
}

void Scheduler::printUserSpaceTraces()
{
  lockScheduling();
  debug(USERTRACE, "Scheduling all userspace threads to print a stacktrace\n");
  for (ustl::list<Thread*>::iterator it = threads_.begin(); it != threads_.end(); ++it)
  {
    Thread *t = *it;
    if (t->user_arch_thread_info_)
    {
      if (t->switch_to_userspace_)
      {
        ArchThreads::changeInstructionPointer(t->kernel_arch_thread_info_, (pointer) printUserSpaceTracesHelper);
        t->switch_to_userspace_ = 0;
      }
      else
      {
        debug(USERTRACE, "Thread <%s> blocked in kernel, printing kernel backtrace instead\n", t->getName());
        t->printBacktrace();
      }
    }
  }
  debug(USERTRACE, "Done scheduling all userspace threads to print a stacktrace\n");
  unlockScheduling();
}

void Scheduler::sleepAndRelease(Lock &lock)
{
  assert(lock.waitersListIsLocked());
  // push back the current thread onto the waiters list
  currentThread->lock_waiting_on_ = &lock;
  lock.pushFrontCurrentThreadToWaitersList();

  lockScheduling();
  currentThread->state_ = Sleeping;
  lock.unlockWaitersList();
  unlockScheduling();
  yield();
}
