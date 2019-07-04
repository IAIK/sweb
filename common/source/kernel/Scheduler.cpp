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

__thread Thread* currentThread = NULL;
__thread ArchThreadRegisters* currentThreadRegisters = NULL;

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
  block_scheduling_ = -1;
  ticks_ = 0;
  addNewThread(&cleanup_thread_);
  //addNewThread(&idle_thread_);
  debug(SCHEDULER, "Initializing scheduler END\n");
}

void Scheduler::schedule()
{
  //debug(SCHEDULER, "CPU %zu, scheduling, currentThread: %p = %s\n", ArchMulticore::getCpuID(), currentThread, currentThread ? currentThread->getName() : "(nil)");

  assert(!ArchInterrupts::testIFSet() && "Tried to schedule with Interrupts enabled");

  if(block_scheduling_.load() == ArchMulticore::getCpuID())
  {
    debug(SCHEDULER_LOCK, "CPU %zu schedule: currently blocked by thread on own cpu\n", ArchMulticore::getCpuID());
    return;
  }

  lockScheduling(DEBUG_STR_HERE);


  Thread* previousThread = currentThread;

  assert(block_scheduling_.load() == ArchMulticore::getCpuID());

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

  debug(SCHEDULER, "schedule CPU %zu, currentThread %s (%p) -> %s (%p)\n", ArchMulticore::getCpuID(),
        (previousThread ? previousThread->getName() : "(nil)"), previousThread,
        (currentThread ? currentThread->getName() : "(nil)"), currentThread);

  ArchThreads::switchToAddressSpace(currentThread);

  currentThread->currently_scheduled_on_cpu_ = ArchMulticore::getCpuID();

  if((it != threads_.end()) && ((it + 1) != threads_.end()))
  {
    assert(it != threads_.end());
    ustl::rotate(threads_.begin(), it + 1, threads_.end()); // no new/delete here - important because interrupts are disabled
  }

  unlockScheduling(DEBUG_STR_HERE);

  //debug(SCHEDULER, "CPU %zu, new currentThread is %p %s, userspace: %d\n", ArchMulticore::getCpuID(), currentThread, currentThread->getName(), currentThread->switch_to_userspace_);

  currentThreadRegisters = (currentThread->switch_to_userspace_ ? currentThread->user_registers_ :
                                                                  currentThread->kernel_registers_);
}

void Scheduler::addNewThread(Thread *thread)
{
  assert(thread);
  debug(SCHEDULER, "addNewThread: %p  %zd:%s\n", thread, thread->getTID(), thread->getName());
  if (currentThread)
    ArchThreads::debugCheckNewThread(thread);
  KernelMemoryManager::instance()->getKMMLock().acquire();
  lockScheduling(DEBUG_STR_HERE);
  KernelMemoryManager::instance()->getKMMLock().release();
  threads_.push_back(thread);
  ++num_threads;
  unlockScheduling(DEBUG_STR_HERE);
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

  lockScheduling(DEBUG_STR_HERE);
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
  unlockScheduling(DEBUG_STR_HERE);
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
  lockScheduling(DEBUG_STR_HERE);
  debug(SCHEDULER, "Scheduler::printThreadList: %zd Threads in List\n", threads_.size());
  for (size_t c = 0; c < threads_.size(); ++c)
    debug(SCHEDULER, "Scheduler::printThreadList: threads_[%zd]: %p  %zd:%s     [%s] at saved %s rip %p\n", c, threads_[c],
          threads_[c]->getTID(), threads_[c]->getName(), Thread::threadStatePrintable[threads_[c]->state_],
          (threads_[c]->switch_to_userspace_ ? "user" : "kernel"),
          (void*)(threads_[c]->switch_to_userspace_ ? ArchThreads::getInstructionPointer(threads_[c]->user_registers_) : ArchThreads::getInstructionPointer(threads_[c]->kernel_registers_)));
  unlockScheduling(DEBUG_STR_HERE);
}


void Scheduler::lockScheduling(const char* called_at) //not as severe as stopping Interrupts
{
  // This function is used with interrupts enabled, so setting the lock + information about which CPU is holding the lock needs to be atomic

  {
    WithDisabledInterrupts d; // CPU must not change between reading CPU ID and setting the lock value
    size_t expected = -1;
    size_t cpu_id = ArchMulticore::getCpuID();

    do
    {
      ((char*)ArchCommon::getFBPtr())[80*2 + cpu_id*2] = '#';

      if(expected == cpu_id)
      {
        kprintfd("ERROR lockScheduling by %s (%p) on CPU %zu: already locked by own thread at %s , called at %s\n" , (currentThread ? currentThread->getName() : "(nil)"), currentThread, cpu_id, (locked_at_ ? locked_at_ : "(nil)"), called_at);
      }
      assert(expected != cpu_id);

      expected = -1;
      assert(!ArchInterrupts::testIFSet());
    }
    while(!block_scheduling_.compare_exchange_weak(expected, cpu_id));

    ((char*)ArchCommon::getFBPtr())[80*2 + cpu_id*2] = '-';
  }


  scheduling_blocked_by_ = currentThread;
  locked_at_ = (volatile char*)called_at;
  debug(SCHEDULER_LOCK, "locked by %s (%p) on CPU %zu at %s\n", (currentThread ? currentThread->getName() : "(nil)"), currentThread, ArchMulticore::getCpuID(), called_at);
}

void Scheduler::unlockScheduling(const char* called_at)
{
  size_t cpu_id = ArchMulticore::getCpuID();
  if(currentThread)
  {
    debug(SCHEDULER_LOCK, "scheduling unlocked by %s (%p) on CPU %zu at %s\n", currentThread->getName(), currentThread, cpu_id, called_at);
  }
  scheduling_blocked_by_ = nullptr;
  locked_at_ = nullptr;
  ((char*)ArchCommon::getFBPtr())[80*2 + cpu_id*2] = ' ';
  size_t was_locked_by = block_scheduling_.exchange(-1);
  if(was_locked_by != cpu_id)
  {
    kprintfd("Scheduling unlocked by CPU %zx, but was locked by CPU %zx\n", cpu_id, was_locked_by);
  }
  assert(was_locked_by == cpu_id);
}

bool Scheduler::isSchedulingEnabled()
{
  if (this)
    return (block_scheduling_.load() == (size_t)-1);
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
  lockScheduling(DEBUG_STR_HERE);
  debug(BACKTRACE, "printing the backtraces of <%zd> threads:\n", threads_.size());

  for (ustl::list<Thread*>::iterator it = threads_.begin(); it != threads_.end(); ++it)
  {
    (*it)->printBacktrace();
    debug(BACKTRACE, "\n");
    debug(BACKTRACE, "\n");
  }

  unlockScheduling(DEBUG_STR_HERE);
}

void Scheduler::printLockingInformation()
{
  size_t thread_count;
  Thread* thread;
  lockScheduling(DEBUG_STR_HERE);
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
      debug(LOCK, "Thread %s (%p) is waiting on lock: %s (%p), held by: %p, last accessed at %zx\n", thread->getName(), thread, thread->lock_waiting_on_ ->getName(), thread->lock_waiting_on_, thread->lock_waiting_on_->heldBy(), thread->lock_waiting_on_->last_accessed_at_);
            thread->lock_waiting_on_->printStatus();
    }
  }
  debug(LOCK, "Scheduler::printLockingInformation finished\n");
  unlockScheduling(DEBUG_STR_HERE);
}

bool Scheduler::isInitialized()
{
        return instance_ != 0;
}
