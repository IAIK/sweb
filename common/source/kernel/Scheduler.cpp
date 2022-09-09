#include "Scheduler.h"
#include "Thread.h"
#include "panic.h"
#include "ArchThreads.h"
#include "ArchCommon.h"
#include "kprintf.h"
#include "ArchInterrupts.h"
#include "KernelMemoryManager.h"
#include "EASTL/list.h"
#include "backtrace.h"
#include "ArchThreads.h"
#include "Mutex.h"
#include "EASTL/map.h"
#include "EASTL/string.h"
#include "EASTL/iterator.h"
#include "Lock.h"
#include "ArchMulticore.h"
#include "Loader.h"
#include "ArchCommon.h"

__cpu Thread* currentThread = nullptr;
__cpu ArchThreadRegisters* currentThreadRegisters = nullptr;

cpu_local IdleThread* idle_thread;

__cpu size_t cpu_ticks = 0;

__cpu eastl::atomic<size_t> preempt_protect_count_ = {};

Scheduler *Scheduler::instance_ = nullptr;

Scheduler *Scheduler::instance()
{
  if (unlikely(!instance_))
    instance_ = new Scheduler();
  return instance_;
}

Scheduler::Scheduler()
{
  debug(SCHEDULER, "Initializing scheduler\n");
  ticks_ = 0;
  addNewThread(&cleanup_thread_);
  debug(SCHEDULER, "Initializing scheduler END\n");
}

void Scheduler::schedule()
{
  if(SCHEDULER & OUTPUT_ADVANCED)
  {
    debug(SCHEDULER, "CPU %zu, scheduling, currentThread: %p = %s\n", SMP::currentCpuId(), currentThread, currentThread ? currentThread->getName() : "(nil)");
  }

  if (preempt_protect_count_.load() > 0)
  {
      kprintfd("Re-Schedule blocked (preemption disabled)\n");
      return;
  }

  assert(!ArchInterrupts::testIFSet() && "Tried to schedule with Interrupts enabled");

  if(scheduler_lock_.isHeldBy(SMP::currentCpuId()))
  {
    debug(SCHEDULER_LOCK, "CPU %zu schedule: currently blocked by thread on own cpu\n", SMP::currentCpuId());
    return;
  }

  uint64 now = ArchCommon::cpuTimestamp();

  scheduler_lock_.acquire();


  Thread* previousThread = currentThread;

  assert(scheduler_lock_.isHeldBy(SMP::currentCpuId()));

  if(previousThread)
  {
    assert(previousThread->isCurrentlyScheduledOnCpu(SMP::currentCpuId()));

    // Increase virtual running time of the thread by the difference between last schedule and now
    updateVruntime(previousThread, now);

    // Threads that yielded their time (without waiting on a lock) are moved to the back of the list by increasing their virtual running time to that of the longest (virtually) running thread
    // Better: increase vruntime by the time slice that would have been allocated for the thread (requires an actual time base and not just cpu timestamps)
    if(previousThread->yielded)
    {
        Thread* max_vruntime_thread = maxVruntimeThread();
        uint64 new_vruntime = max_vruntime_thread->vruntime + 1;
        if (SCHEDULER & OUTPUT_ADVANCED)
        {
            debug(SCHEDULER, "%s yielded while running, increasing vruntime %" PRIu64 " -> %" PRIu64 " (after %s)\n", currentThread->getName(), currentThread->vruntime, new_vruntime, max_vruntime_thread->getName());
        }
        setThreadVruntime(previousThread, eastl::max(previousThread->vruntime, new_vruntime));

        previousThread->yielded = false;
    }

    previousThread->currently_scheduled_on_cpu_ = (size_t)-1;
  }

  assert(!threads_.empty());

  // Pick the thread with the lowest virtual running time (that is schedulable and not already running)
  Thread* min_vruntime_thread = nullptr;
  auto it = threads_.begin();
  for(; it != threads_.end(); ++it)
  {
    bool already_running = (*it)->isCurrentlyScheduled(); // Prevent scheduling threads on multiple CPUs simultaneously
    bool schedulable = (*it)->schedulable();
    bool can_run_on_cpu = (*it)->canRunOnCpu(SMP::currentCpuId());
    bool just_woken = schedulable && !(*it)->prev_schedulable;

    (*it)->prev_schedulable = schedulable;

    if(SCHEDULER & OUTPUT_ADVANCED)
    {
        debug(SCHEDULER, "Check thread (%p) %s, schedulable: %u, just woken: %u, already running: %u, vruntime: %" PRIu64 "\n", *it, (*it)->getName(), schedulable, just_woken, already_running, (*it)->vruntime);
    }

    if(!already_running && schedulable && can_run_on_cpu)
    {
        min_vruntime_thread = *it;

        // Relative virtual running time for threads that have just woken up is set to same as thread with least running time
        // (i.e., schedule them asap, but don't let them run for a really long time to 'catch up' the difference)
        if(just_woken)
        {
            auto min_non_woken = eastl::find_if(eastl::next(it), threads_.end(), [](Thread* t){ return t->schedulable() && t->prev_schedulable; });

            if(min_non_woken != threads_.end())
            {
                auto adjusted_vruntime = eastl::max((*it)->vruntime, (*min_non_woken)->vruntime);
                setThreadVruntime(it, adjusted_vruntime); // Invalidates iterator
            }
        }

        break;
    }
  }

  assert(it != threads_.end());
  assert(min_vruntime_thread);

  currentThread = min_vruntime_thread;

  // auto it = threads_.begin();
  // for(; it != threads_.end(); ++it)
  // {
  //   if((*it)->schedulable())
  //   {
  //     currentThread = *it;
  //     break;
  //   }
  // }



  debug(SCHEDULER, "schedule CPU %zu, currentThread %s (%p) -> %s (%p)\n", SMP::currentCpuId(),
        (previousThread ? previousThread->getName() : "(nil)"), previousThread,
        (currentThread ? currentThread->getName() : "(nil)"), currentThread);

  assert(currentThread);
  assert(currentThread->schedulable());
  assert(!currentThread->isCurrentlyScheduled());


  ArchThreads::switchToAddressSpace(currentThread);

  currentThread->currently_scheduled_on_cpu_ = SMP::currentCpuId();

  // if((it != threads_.end()) && ((it + 1) != threads_.end()))
  // {
  //   assert(it != threads_.end());
  //   eastl::rotate(threads_.begin(), it + 1, threads_.end()); // no new/delete here - important because interrupts are disabled
  // }

  scheduler_lock_.release();

  //debug(SCHEDULER, "CPU %zu, new currentThread is %p %s, userspace: %d\n", ArchMulticore::getCpuID(), currentThread, currentThread->getName(), currentThread->switch_to_userspace_);

  currentThreadRegisters = (currentThread->switch_to_userspace_ ? currentThread->user_registers_ :
                                                                  currentThread->kernel_registers_);

  currentThread->setSchedulingStartTimestamp(ArchCommon::cpuTimestamp());
}

void Scheduler::addNewThread(Thread *thread)
{
  assert(thread);
  debug(SCHEDULER, "addNewThread: %p  %zd:%s\n", thread, thread->getTID(), thread->getName());
  if (currentThread)
    ArchThreads::debugCheckNewThread(thread);
  // Inserting a new thread into the thread list requires allocations
  // -> ensure we won't block on the KMM lock with scheduling disabled
  KernelMemoryManager::instance()->getKMMLock().acquire();
  scheduler_lock_.acquire();
  KernelMemoryManager::instance()->getKMMLock().release();

  auto min_thread = minVruntimeThread();
  if(min_thread)
  {
      thread->vruntime = min_thread->vruntime;
      debug(SCHEDULER, "vruntime for %s = %" PRIu64 "\n", thread->getName(), thread->vruntime);
  }

  threads_.insert(thread);
  ++num_threads;
  scheduler_lock_.release();
}

void Scheduler::sleep(bool should_yield)
{
  currentThread->setState(Thread::Sleeping);
  if (should_yield)
  {
      yield();
  }
}

void Scheduler::wake(Thread* thread_to_wake)
{
  assert(thread_to_wake->getState() == Thread::Sleeping);
  thread_to_wake->setState(Thread::Running);
}

void Scheduler::yield()
{
  assert(this);
  assert(currentThread);

  if (preempt_protect_count_.load() > 0)
  {
      kprintfd("Yield blocked (preemption disabled)\n");
      return;
  }

  if (!ArchInterrupts::testIFSet())
  {
    kprintfd("Scheduler::yield: WARNING Interrupts disabled, do you really want to yield ? (currentThread %p %s)\n",
             currentThread, currentThread->name_.c_str());
    currentThread->printBacktrace();
  }

  if(SCHEDULER & OUTPUT_ADVANCED)
      debug(SCHEDULER, "%s yielded\n", currentThread->getName());

  if (currentThread->getState() == Thread::Running)
  {
      currentThread->yielded = true;
  }

  ArchThreads::yield();
}

void Scheduler::cleanupDeadThreads()
{
  /* Before adding new functionality to this function, consider if that
     functionality could be implemented more cleanly in another place.
     (e.g. Thread/Process destructor) */

  scheduler_lock_.acquire();
  uint32 thread_count_max = threads_.size();
  if (thread_count_max > 1024)
    thread_count_max = 1024;
  Thread* destroy_list[thread_count_max];
  uint32 thread_count = 0;

  auto it = threads_.begin();
  while(it != threads_.end())
  {
    Thread* t = *it;
    if ((t->getState() == Thread::ToBeDestroyed) && !t->isCurrentlyScheduled())
    {
      destroy_list[thread_count++] = t;
      it = threads_.erase(it); // Note: erase will not realloc!
      --num_threads;

      if (thread_count >= thread_count_max)
        break;
    }
    else
      ++it;
  }
  scheduler_lock_.release();

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
  scheduler_lock_.acquire();
  kprintfd("Scheduler::printThreadList: %zd Threads in List\n", threads_.size());
  for (auto t : threads_)
  {
      kprintfd("Scheduler::printThreadList: %p  %zd:%s     [%s] at saved %s rip %p, vruntime: %" PRIu64 "\n", t,
            t->getTID(), t->getName(), Thread::threadStatePrintable[t->state_],
            (t->switch_to_userspace_ ? "user" : "kernel"),
            (void*)(t->switch_to_userspace_ ? ArchThreads::getInstructionPointer(t->user_registers_) : ArchThreads::getInstructionPointer(t->kernel_registers_)), t->vruntime);
  }
  scheduler_lock_.release();
}

bool Scheduler::isSchedulingEnabled()
{
  if (this)
    return scheduler_lock_.heldBy() == (size_t)-1;
  else
    return false;
}

bool Scheduler::isCurrentlyCleaningUp()
{
  return currentThread == &cleanup_thread_;
}

uint32 Scheduler::getTicks() const
{
  return ticks_;
}

uint32 Scheduler::getCpuTicks() const
{
  return cpu_ticks;
}

void Scheduler::incTicks()
{
  ++ticks_;
}

void Scheduler::incCpuTicks()
{
  ++cpu_ticks;
}

void Scheduler::printStackTraces()
{
  scheduler_lock_.acquire();
  debug(BACKTRACE, "printing the backtraces of <%zd> threads:\n", threads_.size());

  for (auto & thread : threads_)
  {
    thread->printBacktrace();
    debug(BACKTRACE, "\n");
    debug(BACKTRACE, "\n");
  }

  scheduler_lock_.release();
}

void Scheduler::printLockingInformation()
{
  scheduler_lock_.acquire();
  kprintfd("\n");
  debug(LOCK, "Scheduler::printLockingInformation:\n");
  for (auto thread : threads_)
  {
    if(thread->holding_lock_list_ != nullptr)
    {
      Lock::printHoldingList(thread);
    }
  }
  for (auto thread : threads_)
  {
    if(thread->lock_waiting_on_ != nullptr)
    {
      debug(LOCK, "Thread %s (%p) is waiting on lock: %s (%p), held by: %p, last accessed at %zx\n", thread->getName(), thread, thread->lock_waiting_on_ ->getName(), thread->lock_waiting_on_, thread->lock_waiting_on_->heldBy(), thread->lock_waiting_on_->last_accessed_at_);
            thread->lock_waiting_on_->printStatus();
    }
  }
  debug(LOCK, "Scheduler::printLockingInformation finished\n");
  scheduler_lock_.release();
}

bool Scheduler::isInitialized()
{
    return instance_ != nullptr;
}

Thread* Scheduler::minVruntimeThread()
{
    assert(scheduler_lock_.isHeldBy(SMP::currentCpuId()));
    for(auto & thread : threads_)
    {
        if(thread->schedulable())
        {
            return thread;
        }
    }

    return nullptr;
}

Thread* Scheduler::maxVruntimeThread()
{
    assert(scheduler_lock_.isHeldBy(SMP::currentCpuId()));
    for(auto it = threads_.rbegin(); it != threads_.rend(); ++it)
    {
        if((*it)->schedulable())
        {
            return *it;
        }
    }

    return nullptr;
}

void Scheduler::updateVruntime(Thread* t, uint64 now)
{
    assert(t->currently_scheduled_on_cpu_ == SMP::currentCpuId());

    if(now <= t->schedulingStartTimestamp())
    {
        return;
    }

    uint64 time_delta = now - t->schedulingStartTimestamp();

    setThreadVruntime(t, t->vruntime + time_delta);


    if(SCHEDULER & OUTPUT_ADVANCED)
    {
        debug(SCHEDULER, "CPU %zu, %s vruntime: %" PRIu64 " (+ %" PRIu64 ") [%" PRIu64 " -> %" PRIu64 "]\n", SMP::currentCpuId(), t->getName(), t->vruntime, time_delta, t->schedulingStartTimestamp(), now);
    }

    t->setSchedulingStartTimestamp(now);
}

void Scheduler::setThreadVruntime(Thread* t, uint64 new_vruntime)
{
    assert(scheduler_lock_.isHeldBy(SMP::currentCpuId()));

    auto it = threads_.find(t);

    setThreadVruntime(it, new_vruntime);
}

void Scheduler::setThreadVruntime(Scheduler::ThreadList::iterator it, uint64 new_vruntime)
{
    assert(scheduler_lock_.isHeldBy(SMP::currentCpuId()));
    assert(it != threads_.end());
    Thread* t = *it;
    if(SCHEDULER & OUTPUT_ADVANCED)
    {
        debug(SCHEDULER, "CPU %zu, set %s vruntime = %" PRIu64 "\n", SMP::currentCpuId(), t->getName(), new_vruntime);
    }

    threads_.erase(it);

    t->vruntime = new_vruntime;

    threads_.insert(t);
}
