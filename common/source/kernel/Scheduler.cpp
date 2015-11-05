/**
 * @file Scheduler.cpp
 */

#include "Scheduler.h"
#include "Thread.h"
#include "panic.h"
#include "ArchThreads.h"
#include "ArchCommon.h"
#include "console/kprintf.h"
#include "ArchInterrupts.h"
#include "mm/KernelMemoryManager.h"
#include <ustl/ulist.h>
#include "backtrace.h"
#include "ArchThreads.h"

#include "ustl/umap.h"
#include "ustl/ustring.h"

extern ustl::map<uint32, ustl::string> symbol_table;

__thread ArchThreadInfo *currentThreadInfo;
__thread Thread *currentThread;
__thread ThreadState* lastThreadState;
__thread uint32 core;

Scheduler *Scheduler::instance_=0;

Scheduler *Scheduler::instance()
{
  return instance_;
}

/**
 * @class IdleThread
 * periodically calls cleanUpDeadThreads
 */
class IdleThread : public Thread
{
  public:

    /**
     * Constructor
     * @return IdleThread instance
     */
    IdleThread() : Thread("IdleThread")
    {
    }

    /**
     * calls cleanUpDeadThreads
     */
    virtual void Run()
    {
      uint32 last_ticks = 0;
      uint32 new_ticks = 0;
      while ( 1 )
      {
    	debug ( SCHEDULER,"Idle Thread cleans up dead threads\n");
        Scheduler::instance()->cleanupDeadThreads();
        new_ticks = Scheduler::instance()->getTicks();
        debug ( SCHEDULER,"Idle Thread finished cleaning up dead threads\n");
        if (new_ticks == last_ticks)
        {
          last_ticks = new_ticks + 1;
          debug ( SCHEDULER,"Idle Thread is idle\n");
          ArchCommon::idle();
        }
        else
        {
          last_ticks = new_ticks;
          debug ( SCHEDULER,"Idle Thread yields\n");
          Scheduler::instance()->yield();
        }
      }
    }
};

void Scheduler::createScheduler()
{
  if (instance_)
    return;

  instance_ = new Scheduler();

  // create idle thread, this one really does not do too much

  // TODO: give every CPU an IdleThread!!
  Thread *idle = new IdleThread();
  instance_->addNewThread ( idle );
}

Scheduler::Scheduler()
{
  block_scheduling_=0;
  ticks_=0;
}

void Scheduler::addNewThread ( Thread *thread )
{
  debug ( SCHEDULER,"addNewThread: %x  %d:%s\n",thread,thread->getPID(), thread->getName() );
  lockScheduling();
  waitForFreeSpinLock(KernelMemoryManager::instance()->getKMMLock());
  threads_.push_back ( thread );
  unlockScheduling();
  debug ( SCHEDULER,"finished addNewThread\n");
}

void Scheduler::removeCurrentThread()
{
  debug ( SCHEDULER,"removeCurrentThread: %x %d:%s, threads_.size() %d\n",currentThread,currentThread->getPID(),currentThread->getName(),threads_.size() );
  lockScheduling();
  waitForFreeSpinLock(KernelMemoryManager::instance()->getKMMLock());
  if ( threads_.size() > 1 )
  {
    threads_.remove(currentThread);
  }
  unlockScheduling();
}

void Scheduler::sleep()
{
  currentThread->state_=Sleeping;
  assert(block_scheduling_ == 0);
  yield();
}

void Scheduler::sleepAndRelease ( SpinLock &lock )
{
  lockScheduling();
  currentThread->state_=Sleeping;
  lock.release();
  unlockScheduling();
  yield();
}

void Scheduler::sleepAndRelease ( Mutex &lock )
{
  lockScheduling();
  waitForFreeSpinLock(lock.spinlock_);
  currentThread->state_=Sleeping;
  lock.release();
  unlockScheduling();
  yield();
}

// beware using this funktion!
void Scheduler::sleepAndRestoreInterrupts ( bool interrupts )
{
  // why do it get the feeling that misusing this function
  // can mean a serious shot in the foot ?
  currentThread->state_=Sleeping;
  if ( interrupts )
  {
    //assert(block_scheduling_ == 0);
    ArchInterrupts::enableInterrupts();
    yield();
  }
}

void Scheduler::wake ( Thread* thread_to_wake )
{
  thread_to_wake->state_=Ready;
}

uint32 Scheduler::schedule()
{

  while (!lockScheduling(1))
  {
	  if (currentThread && currentThread->state_ == Running)
		  return currentThread->switch_to_userspace_;
  }


  uint32 local_gs;
  asm volatile("xor %%eax,%%eax\n"
		       "mov %%gs,%%ax\n"
               "mov %%eax,%0\n": "=m" (local_gs) :: "eax");


  Thread* previousThread = currentThread;
  lastThreadState = &previousThread->state_;
  size_t num_threads = 0;

  do
  {
    // WARNING: do not read currentThread before is has been set here
    //          the first time scheduler is called.
    //          before this, currentThread may be 0 !!

    currentThread = threads_.front();
    num_threads++;

    //this operation doesn't allocate or delete any kernel memory (important because Interrupts are disabled in this method)
    ustl::rotate(threads_.begin(),threads_.begin()+1, threads_.end());

    // TODO: is it save to use num_threads + threads_.size() here instead of currentThread == previousThread????????
    if ((num_threads == threads_.size()) && !((currentThread->state_ == Running) || (currentThread->state_ == Ready)))
    {
      debug(SCHEDULER, "Scheduler::schedule: ERROR: currentThread == previousThread! Either no thread is in state Running or you added the same thread more than once.");
      printThreadListNoLocking();
      assert(false);
    }

    //printThreadListNoLocking();

  }
  while (currentThread->state_ != Ready);

  currentThread->state_ = Running;



  unlockScheduling();


  uint32 ret = 1;

  debug ( SCHEDULER,"Scheduler::schedule: core:%x new currentThread is %x %s, switch_userspace:%d\n",local_gs, currentThread,currentThread ? currentThread->getName() : 0,currentThread ? currentThread->switch_to_userspace_ : 3);

  //debug ( SCHEDULER,"Scheduler::schedule: gs:%x\n",local_gs);


  if ( currentThread->switch_to_userspace_ )
    currentThreadInfo =  currentThread->user_arch_thread_info_;
  else
  {
    currentThreadInfo =  currentThread->kernel_arch_thread_info_;
    ret=0;
  }
 // debug ( SCHEDULER,"Scheduler::schedule: new currentThreadInfo: %x\n", currentThreadInfo);

  currentThreadInfo->gs = local_gs;
  currentThreadInfo->gs0 = local_gs;

  debug ( SCHEDULER,"Scheduler::schedule: finished schedule()\n");


  return ret;
}

void Scheduler::yield()
{
  assert(this);
  if ( ! ArchInterrupts::testIFSet() )
  {
    assert(currentThread);
    kprintfd ( "Scheduler::yield: WARNING Interrupts disabled, do you really want to yield ? (currentThread %x %s)\n", currentThread, currentThread->name_ );
    kprintf ( "Scheduler::yield: WARNING Interrupts disabled, do you really want to yield ?\n" );
    currentThread->printBacktrace();
  }
  ArchThreads::yield();
  //kprintf ( "Scheduler::yield\n" );
}

bool Scheduler::checkThreadExists ( Thread* thread )
{
  bool retval=false;
  lockScheduling();
  for ( uint32 c=0; c<threads_.size();++c ) //fortunately this doesn't involve KMM
    if ( threads_[c]==thread )
    {
      retval=true;
      break;
    }
  unlockScheduling();
  return retval;
}

void Scheduler::cleanupDeadThreads()
{
  lockScheduling();
  uint32 thread_count_max = threads_.size();
  if (thread_count_max > 1024)
    thread_count_max = 1024;
  Thread* destroy_list[thread_count_max];
  uint32 thread_count = 0;
  for(uint32 i = 0; i < threads_.size(); ++i)
  {
    Thread* tmp = threads_[i];
    if(tmp->state_ == ToBeDestroyed)
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
    for(uint32 i = 0; i < thread_count; ++i)
    {
      delete destroy_list[i];
    }
    debug ( SCHEDULER, "cleanupDeadThreads: done\n" );
  }
}

void Scheduler::printThreadList()
{
  uint32 c=0;
  lockScheduling();
  debug ( SCHEDULER, "Scheduler::printThreadList: %d Threads in List\n",threads_.size() );
  for ( c=0; c<threads_.size();++c )
    debug ( SCHEDULER, "Scheduler::printThreadList: threads_[%d]: %x  %d:%s     [%s]\n",c,threads_[c],threads_[c]->getPID(),threads_[c]->getName(),Thread::threadStatePrintable[threads_[c]->state_] );
  unlockScheduling();
}

void Scheduler::printThreadListNoLocking()
{
  uint32 c=0;
  debug ( SCHEDULER, "Scheduler::printThreadList: %d Threads in List\n",threads_.size() );
  for ( c=0; c<threads_.size();++c )
    debug ( SCHEDULER, "Scheduler::printThreadList: threads_[%d]: %x  %d:%s     [%s]\n",c,threads_[c],threads_[c]->getPID(),threads_[c]->getName(),Thread::threadStatePrintable[threads_[c]->state_] );
}

bool Scheduler::lockScheduling(uint32 by_schedule)  //not as severe as stopping Interrupts
{
  return ! unlikely ( ArchThreads::testSetLock ( block_scheduling_,1 ) );

}

void Scheduler::unlockScheduling()
{
  //scheduling_blocked_by_schedule_ = 0;
  block_scheduling_ = 0;
}

void Scheduler::waitForFreeSpinLock(SpinLock& lock)  //not as severe as stopping Interrupts
{
  if ( block_scheduling_==0 )
    kpanict ( ( uint8* ) "FATAL ERROR: Scheduler::waitForFreeSpinLock: This "
                            "is meant to be used while Scheduler is locked\n" );

  uint32 ticks = 0;
  while (!lock.isFree())
  {
    if (unlikely(++ticks > 50))
    {
      kprintfd("WARNING: Scheduler::waitForFreeSpinLock: SpinLock <%s> is locked since more than %d ticks? Maybe there is something wrong!\n", lock.name_,ticks);
      Thread* t = lock.heldBy();
      kprintfd("Thread holding SpinLock: %x  %d:%s     [%s]\n",t,t->getPID(),t->getName(),Thread::threadStatePrintable[t->state_]);
      t->printBacktrace();
      //assert(false);
    }
    unlockScheduling();
    yield();
    lockScheduling();
  }
}

bool Scheduler::isSchedulingEnabled()
{
  if (this)
    return (block_scheduling_ == 0);
  else
    return false;
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
