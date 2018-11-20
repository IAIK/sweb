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
#include "ArchMulticore.h"

__thread Thread* currentThread = NULL;
__thread ArchThreadRegisters* currentThreadRegisters = NULL;

__thread char scheduling_stack[PAGE_SIZE];

thread_local IdleThread idle_thread;

Scheduler *Scheduler::instance_ = 0;

Scheduler *Scheduler::instance()
{
  if (unlikely(!instance_))
    instance_ = new Scheduler();
  return instance_;
}

Scheduler::Scheduler()
{
  debug(SCHEDULER, "Initializing scheduler\n");
  block_scheduling_ = 0;
  ticks_ = 0;
  addNewThread(&cleanup_thread_);
  //addNewThread(&idle_thread_);
  debug(SCHEDULER, "Initializing scheduler END\n");
}

uint32 Scheduler::schedule()
{
  //debug(SCHEDULER, "CPU %zu, scheduling, currentThread: %p = %s\n", ArchMulticore::getCpuID(), currentThread, currentThread ? currentThread->getName() : "(nil)");

  assert(!ArchInterrupts::testIFSet() && "Tried to schedule with Interrupts enabled");
  if(currentThread && !(currentThread->getState() == ToBeDestroyed))
  {
    if (!tryLockScheduling())
    {
      debug(SCHEDULER, "CPU %zu schedule: currently blocked\n", ArchMulticore::getCpuID());
      return 0;
    }
  }
  else
  {
    lockScheduling();
  }

  assert(block_scheduling_ == 1);

  if(currentThread)
  {
    assert(currentThread->currently_scheduled_on_cpu_ == ArchMulticore::getCpuID());
    currentThread->currently_scheduled_on_cpu_ = (size_t)-1;
  }

  assert(threads_.size() != 0);

  auto it = threads_.begin();
  for(; it != threads_.end(); ++it)
  {
    if((*it)->schedulable())
    {
      currentThread = *it;
      break;
    }
  }

  if(it == threads_.end())
  {
          assert(idle_thread.schedulable());
          currentThread = &idle_thread;
  }

  assert(currentThread);
  assert(currentThread->currently_scheduled_on_cpu_ == (size_t)-1);

  // TODO: We are still using the kernel stack of the thread we just de-scheduled -> other cpu can schedule thread and overwrite the stack we are currently still using

  currentThread->currently_scheduled_on_cpu_ = ArchMulticore::getCpuID();

  ustl::rotate(threads_.begin(), it + 1, threads_.end()); // no new/delete here - important because interrupts are disabled

  unlockScheduling();

  //debug(SCHEDULER, "CPU %zu, new currentThread is %p %s, userspace: %d\n", ArchMulticore::getCpuID(), currentThread, currentThread->getName(), currentThread->switch_to_userspace_);

  uint32 ret = 1;

  if (currentThread->switch_to_userspace_)
  {
    currentThreadRegisters = currentThread->user_registers_;
  }
  else
  {
    currentThreadRegisters = currentThread->kernel_registers_;
    ret = 0;
  }

  return ret;
}

void Scheduler::addNewThread(Thread *thread)
{
  //cpu_scheduler.addNewThread(thread);
  assert(thread);
  debug(SCHEDULER, "addNewThread: %p  %zd:%s\n", thread, thread->getTID(), thread->getName());
  if (currentThread)
    ArchThreads::debugCheckNewThread(thread);
  KernelMemoryManager::instance()->getKMMLock().acquire();
  lockScheduling();
  KernelMemoryManager::instance()->getKMMLock().release();
  threads_.push_back(thread);
  unlockScheduling();
}

void Scheduler::sleep()
{
  currentThread->setState(Sleeping);
  yield();
}

void Scheduler::wake(Thread* thread_to_wake)
{
  // wait until the thread is sleeping
  while(thread_to_wake->getState() != Sleeping)
    yield();
  thread_to_wake->setState(Running);
}

void Scheduler::yield()
{
  assert(this);
  if (!ArchInterrupts::testIFSet())
  {
    assert(currentThread);
    kprintfd("Scheduler::yield: WARNING Interrupts disabled, do you really want to yield ? (currentThread %p %s)\n",
             currentThread, currentThread->name_.c_str());
    currentThread->printBacktrace();
  }
  ArchThreads::yield();
}

void Scheduler::cleanupDeadThreads()
{
  /* Before adding new functionality to this function, consider if that
     functionality could be implemented more cleanly in another place.
     (e.g. Thread/Process destructor) */

  //debug(SCHEDULER, "cleanupDeadThreads lock scheduling\n");
  lockScheduling();
  uint32 thread_count_max = threads_.size();
  if (thread_count_max > 1024)
    thread_count_max = 1024;
  Thread* destroy_list[thread_count_max];
  uint32 thread_count = 0;
  for (uint32 i = 0; i < threads_.size(); ++i)
  {
    Thread* tmp = threads_[i];
    if ((tmp->getState() == ToBeDestroyed) && (tmp->currently_scheduled_on_cpu_ == (size_t)-1))
    {
      destroy_list[thread_count++] = tmp;
      threads_.erase(threads_.begin() + i); // Note: erase will not realloc!
      --i;
    }
    if (thread_count >= thread_count_max)
      break;
  }
  //debug(SCHEDULER, "cleanupDeadThreads unlock scheduling\n");
  unlockScheduling();
  if (thread_count > 0)
  {
    for (uint32 i = 0; i < thread_count; ++i)
    {
      delete destroy_list[i];
    }
    debug(SCHEDULER, "cleanupDeadThreads: done\n");
  }
}

void Scheduler::printThreadList()
{
  lockScheduling();
  debug(SCHEDULER, "Scheduler::printThreadList: %zd Threads in List\n", threads_.size());
  for (size_t c = 0; c < threads_.size(); ++c)
    debug(SCHEDULER, "Scheduler::printThreadList: threads_[%zd]: %p  %zd:%s     [%s]\n", c, threads_[c],
          threads_[c]->getTID(), threads_[c]->getName(), Thread::threadStatePrintable[threads_[c]->state_]);
  unlockScheduling();
}


bool Scheduler::tryLockScheduling()
{
  return ArchThreads::testSetLock(block_scheduling_, 1) == 0;
}

void Scheduler::lockScheduling() //not as severe as stopping Interrupts
{
  while(ArchThreads::testSetLock(block_scheduling_, 1));
  /*
  if (unlikely(ArchThreads::testSetLock(block_scheduling_, 1)))
    kpanict("FATAL ERROR: Scheduler::*: block_scheduling_ was set !! How the Hell did the program flow get here then ?\n");
  */
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
  debug(BACKTRACE, "printing the backtraces of <%zd> threads:\n", threads_.size());

  for (ustl::list<Thread*>::iterator it = threads_.begin(); it != threads_.end(); ++it)
  {
    (*it)->printBacktrace();
    debug(BACKTRACE, "\n");
    debug(BACKTRACE, "\n");
  }

  unlockScheduling();
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
      debug(LOCK, "Thread %s (%p) is waiting on lock: %s (%p).\n", thread->getName(), thread,
            thread->lock_waiting_on_ ->getName(), thread->lock_waiting_on_ );
    }
  }
  debug(LOCK, "Scheduler::printLockingInformation finished\n");
  unlockScheduling();
}

bool Scheduler::isInitialized()
{
        return instance_ != 0;
}
