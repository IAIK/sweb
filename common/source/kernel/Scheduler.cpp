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
#include "Loader.h"
#include "ArchCommon.h"

__thread Thread* currentThread = nullptr;
__thread ArchThreadRegisters* currentThreadRegisters = nullptr;

thread_local IdleThread idle_thread;

__thread size_t cpu_ticks = 0;

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
  // block_scheduling_ = -1;
  ticks_ = 0;
  addNewThread(&cleanup_thread_);
  debug(SCHEDULER, "Initializing scheduler END\n");
}

void Scheduler::schedule()
{
  if(SCHEDULER & OUTPUT_ADVANCED)
  {
    debug(SCHEDULER, "CPU %zu, scheduling, currentThread: %p = %s\n", ArchMulticore::getCpuID(), currentThread, currentThread ? currentThread->getName() : "(nil)");
  }

  assert(!ArchInterrupts::testIFSet() && "Tried to schedule with Interrupts enabled");

  if(scheduler_lock_.isHeldBy(ArchMulticore::getCpuID()))
  // if(block_scheduling_.load() == ArchMulticore::getCpuID())
  {
    debug(SCHEDULER_LOCK, "CPU %zu schedule: currently blocked by thread on own cpu\n", ArchMulticore::getCpuID());
    return;
  }

  uint64 now = ArchCommon::cpuTimestamp();

  // lockScheduling(DEBUG_STR_HERE);
  scheduler_lock_.acquire();


  Thread* previousThread = currentThread;

  // assert(block_scheduling_.load() == ArchMulticore::getCpuID());
  assert(scheduler_lock_.isHeldBy(ArchMulticore::getCpuID()));

  if(currentThread)
  {
    assert(currentThread->isCurrentlyScheduledOnCpu(ArchMulticore::getCpuID()));

    // Increase virtual running time of the thread by the difference between last schedule and now
    updateVruntime(currentThread, now);

    // Threads that yielded their time (without waiting on a lock) are moved to the back of the list by increasing their virtual running time to that of the longest (virtually) running thread
    // Better: increase vruntime by the time slice that would have been allocated for the thread (requires an actual time base and not just cpu timestamps)
    if(currentThread->yielded)
    {
        Thread* max_vruntime_thread = maxVruntimeThread();
        uint64 new_vruntime = max_vruntime_thread->vruntime + 1;
        if (SCHEDULER & OUTPUT_ADVANCED)
        {
            debug(SCHEDULER, "%s yielded while running, increasing vruntime %llu -> %llu (after %s)\n", currentThread->getName(), currentThread->vruntime, new_vruntime, max_vruntime_thread->getName());
        }
        setThreadVruntime(currentThread, ustl::max(currentThread->vruntime, new_vruntime));

        currentThread->yielded = false;
    }

    currentThread->currently_scheduled_on_cpu_ = (size_t)-1;
  }

  assert(!threads_.empty());

  // Pick the thread with the lowest virtual running time (that is schedulable and not already running)
  Thread* min_vruntime_thread = nullptr;
  auto* it = threads_.begin();
  for(; it != threads_.end(); ++it)
  {
    bool already_running = (*it)->isCurrentlyScheduled(); // Prevent scheduling threads on multiple CPUs simultaneously
    bool schedulable = (*it)->schedulable();
    bool can_run_on_cpu = (*it)->canRunOnCpu(ArchMulticore::getCpuID());
    bool just_woken = schedulable && !(*it)->prev_schedulable;

    (*it)->prev_schedulable = schedulable;

    if(SCHEDULER & OUTPUT_ADVANCED)
    {
        debug(SCHEDULER, "Check thread (%p) %s, schedulable: %u, just woken: %u, already running: %u, vruntime: %llu\n", *it, (*it)->getName(), schedulable, just_woken, already_running, (*it)->vruntime);
    }

    if(!already_running && schedulable && can_run_on_cpu)
    {
        min_vruntime_thread = *it;

        // Relative virtual running time for threads that have just woken up is set to same as thread with least running time
        // (i.e., schedule them asap, but don't let them run for a really long time to 'catch up' the difference)
        if(just_woken)
        {
            auto min_non_woken = ustl::find_if(it + 1, threads_.end(), [](Thread* t){ return t->schedulable() && t->prev_schedulable; });

            if(min_non_woken != threads_.end())
            {
                auto adjusted_vruntime = ustl::max((*it)->vruntime, (*min_non_woken)->vruntime);
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



  debug(SCHEDULER, "schedule CPU %zu, currentThread %s (%p) -> %s (%p)\n", ArchMulticore::getCpuID(),
        (previousThread ? previousThread->getName() : "(nil)"), previousThread,
        (currentThread ? currentThread->getName() : "(nil)"), currentThread);

  assert(currentThread);
  assert(currentThread->schedulable());
  assert(!currentThread->isCurrentlyScheduled());


  ArchThreads::switchToAddressSpace(currentThread);

  currentThread->currently_scheduled_on_cpu_ = ArchMulticore::getCpuID();

  // if((it != threads_.end()) && ((it + 1) != threads_.end()))
  // {
  //   assert(it != threads_.end());
  //   ustl::rotate(threads_.begin(), it + 1, threads_.end()); // no new/delete here - important because interrupts are disabled
  // }

  // unlockScheduling(DEBUG_STR_HERE);
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
  // lockScheduling(DEBUG_STR_HERE);
  scheduler_lock_.acquire();
  KernelMemoryManager::instance()->getKMMLock().release();

  auto min_thread = minVruntimeThread();
  if(min_thread)
  {
      thread->vruntime = min_thread->vruntime;
      debug(SCHEDULER, "vruntime for %s = %llu\n", thread->getName(), thread->vruntime);
  }

  threads_.insert(thread);
  ++num_threads;
  // unlockScheduling(DEBUG_STR_HERE);
  scheduler_lock_.release();
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

  if(SCHEDULER & OUTPUT_ADVANCED)
  {
      debug(SCHEDULER, "%s yielded\n", currentThread->getName());
  }

  if (currentThread->getState() == Running)
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

  // lockScheduling(DEBUG_STR_HERE);
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
    if ((t->getState() == ToBeDestroyed) && !t->isCurrentlyScheduled())
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
  // unlockScheduling(DEBUG_STR_HERE);
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
  // lockScheduling(DEBUG_STR_HERE);
  scheduler_lock_.acquire();
  debug(SCHEDULER, "Scheduler::printThreadList: %zd Threads in List\n", threads_.size());
  for (size_t c = 0; c < threads_.size(); ++c)
    debug(SCHEDULER, "Scheduler::printThreadList: threads_[%zd]: %p  %zd:%s     [%s] at saved %s rip %p, vruntime: %llu\n", c, threads_[c],
          threads_[c]->getTID(), threads_[c]->getName(), Thread::threadStatePrintable[threads_[c]->state_],
          (threads_[c]->switch_to_userspace_ ? "user" : "kernel"),
          (void*)(threads_[c]->switch_to_userspace_ ? ArchThreads::getInstructionPointer(threads_[c]->user_registers_) : ArchThreads::getInstructionPointer(threads_[c]->kernel_registers_)), threads_[c]->vruntime);
  // unlockScheduling(DEBUG_STR_HERE);
  scheduler_lock_.release();
}

#define INDICATOR_SCHED_LOCK_FREE ' '
#define INDICATOR_SCHED_LOCK_BLOCKED '#'
#define INDICATOR_SCHED_LOCK_HOLDING '-'

// void Scheduler::lockScheduling(const char* called_at) //not as severe as stopping Interrupts
// {
//   // This function is also used with interrupts enabled, so setting the lock + information about which CPU is holding the lock needs to be atomic

//   {
//     WithDisabledInterrupts d; // CPU must not change between reading CPU ID and setting the lock value
//     size_t expected = -1;
//     size_t cpu_id = ArchMulticore::getCpuID();

//     size_t attempt = 0;

//     do
//     {
//       ((char*)ArchCommon::getFBPtr())[80*2 + cpu_id*2] = INDICATOR_SCHED_LOCK_BLOCKED;

//       ++attempt;
//       if(attempt == 2)
//           ++scheduler_lock_count_blocked;

//       if(expected == cpu_id)
//       {
//         kprintfd("ERROR lockScheduling by %s (%p) on CPU %zu: already locked by own thread at %s , called at %s\n" , (currentThread ? currentThread->getName() : "(nil)"), currentThread, cpu_id, (locked_at_ ? locked_at_ : "(nil)"), called_at);
//       }
//       assert(expected != cpu_id);

//       expected = -1;
//       assert(!ArchInterrupts::testIFSet());
//     }
//     while(!block_scheduling_.compare_exchange_weak(expected, cpu_id));

//     if(attempt == 1)
//         ++scheduler_lock_count_free;

//     ((char*)ArchCommon::getFBPtr())[80*2 + cpu_id*2] = INDICATOR_SCHED_LOCK_HOLDING;
//   }


//   scheduling_blocked_by_ = currentThread;
//   locked_at_ = (volatile char*)called_at;
//   debug(SCHEDULER_LOCK, "locked by %s (%p) on CPU %zu at %s\n", (currentThread ? currentThread->getName() : "(nil)"), currentThread, ArchMulticore::getCpuID(), called_at);
// }

// void Scheduler::unlockScheduling(const char* called_at)
// {
//   size_t cpu_id = ArchMulticore::getCpuID();
//   if(currentThread)
//   {
//     debug(SCHEDULER_LOCK, "scheduling unlocked by %s (%p) on CPU %zu at %s\n", currentThread->getName(), currentThread, cpu_id, called_at);
//   }
//   scheduling_blocked_by_ = nullptr;
//   locked_at_ = nullptr;
//   ((char*)ArchCommon::getFBPtr())[80*2 + cpu_id*2] = INDICATOR_SCHED_LOCK_FREE;
//   size_t was_locked_by = block_scheduling_.exchange(-1);
//   if(was_locked_by != cpu_id)
//   {
//     kprintfd("Scheduling unlocked by CPU %zx, but was locked by CPU %zx\n", cpu_id, was_locked_by);
//   }
//   assert(was_locked_by == cpu_id);
// }

bool Scheduler::isSchedulingEnabled()
{
  if (this)
    // return (block_scheduling_.load() == (size_t)-1);
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
  // lockScheduling(DEBUG_STR_HERE);
  scheduler_lock_.acquire();
  debug(BACKTRACE, "printing the backtraces of <%zd> threads:\n", threads_.size());

  for (auto & thread : threads_)
  {
    thread->printBacktrace();
    debug(BACKTRACE, "\n");
    debug(BACKTRACE, "\n");
  }

  // unlockScheduling(DEBUG_STR_HERE);
  scheduler_lock_.release();
}

void Scheduler::printLockingInformation()
{
  size_t thread_count;
  Thread* thread;
  // lockScheduling(DEBUG_STR_HERE);
  scheduler_lock_.acquire();
  kprintfd("\n");
  debug(LOCK, "Scheduler::printLockingInformation:\n");
  for (thread_count = 0; thread_count < threads_.size(); ++thread_count)
  {
    thread = threads_[thread_count];
    if(thread->holding_lock_list_ != nullptr)
    {
      Lock::printHoldingList(threads_[thread_count]);
    }
  }
  for (thread_count = 0; thread_count < threads_.size(); ++thread_count)
  {
    thread = threads_[thread_count];
    if(thread->lock_waiting_on_ != nullptr)
    {
      debug(LOCK, "Thread %s (%p) is waiting on lock: %s (%p), held by: %p, last accessed at %zx\n", thread->getName(), thread, thread->lock_waiting_on_ ->getName(), thread->lock_waiting_on_, thread->lock_waiting_on_->heldBy(), thread->lock_waiting_on_->last_accessed_at_);
            thread->lock_waiting_on_->printStatus();
    }
  }
  debug(LOCK, "Scheduler::printLockingInformation finished\n");
  // unlockScheduling(DEBUG_STR_HERE);
  scheduler_lock_.release();
}

bool Scheduler::isInitialized()
{
    return instance_ != nullptr;
}

Thread* Scheduler::minVruntimeThread()
{
    // assert(block_scheduling_.load() == ArchMulticore::getCpuID());
    assert(scheduler_lock_.isHeldBy(ArchMulticore::getCpuID()));
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
    // assert(block_scheduling_.load() == ArchMulticore::getCpuID());
    assert(scheduler_lock_.isHeldBy(ArchMulticore::getCpuID()));
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
    assert(t->currently_scheduled_on_cpu_ == ArchMulticore::getCpuID());

    if(now <= t->schedulingStartTimestamp())
    {
        return;
    }

    uint64 time_delta = now - t->schedulingStartTimestamp();

    setThreadVruntime(t, t->vruntime + time_delta);


    if(SCHEDULER & OUTPUT_ADVANCED)
    {
        debug(SCHEDULER, "CPU %zu, %s vruntime: %llu (+ %llu) [%llu -> %llu]\n", ArchMulticore::getCpuID(), t->getName(), t->vruntime, time_delta, t->schedulingStartTimestamp(), now);
    }

    t->setSchedulingStartTimestamp(now);
}

void Scheduler::setThreadVruntime(Thread* t, uint64 new_vruntime)
{
    // assert(block_scheduling_.load() == ArchMulticore::getCpuID());
    assert(scheduler_lock_.isHeldBy(ArchMulticore::getCpuID()));

    auto it = threads_.find(t);

    setThreadVruntime(it, new_vruntime);
}

void Scheduler::setThreadVruntime(Scheduler::ThreadList::iterator it, uint64 new_vruntime)
{
    // assert(block_scheduling_.load() == ArchMulticore::getCpuID());
    assert(scheduler_lock_.isHeldBy(ArchMulticore::getCpuID()));
    assert(it != threads_.end());
    Thread* t = *it;
    if(SCHEDULER & OUTPUT_ADVANCED)
    {
        debug(SCHEDULER, "CPU %zu, set %s vruntime = %llu\n", ArchMulticore::getCpuID(),t->getName(), new_vruntime);
    }


    threads_.erase(it);

    t->vruntime = new_vruntime;

    threads_.insert(t);
}
