/**
 * @file Scheduler.cpp
 */

#include "Scheduler.h"
#include "Thread.h"
#include "arch_panic.h"
#include "ArchThreads.h"
#include "console/kprintf.h"
#include "ArchInterrupts.h"
#include "mm/KernelMemoryManager.h"

ArchThreadInfo *currentThreadInfo;
Thread *currentThread;

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
    IdleThread()
    {
      name_="IdleThread";
    }

    /**
     * calls cleanUpDeadThreads
     */
    virtual void Run()
    {
      while ( 1 )
      {
        Scheduler::instance()->cleanupDeadThreads();
        __asm__ __volatile__ ( "hlt" );
      }
    }
};

void Scheduler::createScheduler()
{
  instance_ = new Scheduler();

  // create idle thread, this one really does not do too much

  Thread *idle = new IdleThread();
  instance_->addNewThread ( idle );
}

Scheduler::Scheduler()
{
  kill_old_=false;
  block_scheduling_=0;
  block_scheduling_extern_=0;
}

void Scheduler::addNewThread ( Thread *thread )
{
  //new Thread gets scheduled next
  //also gets added to front as not to interfere with remove or xchange

  lockScheduling();
  kprintfd_nosleep ( "Scheduler::addNewThread: %x %s\n",thread,thread->getName() );
  waitForFreeKMMLock();
  threads_.pushFront ( thread );
  unlockScheduling();
}

void Scheduler::removeCurrentThread()
{
  lockScheduling();
  kprintfd_nosleep ( "Scheduler::removeCurrentThread: %x %s, threads_.size() %d\n",currentThread,currentThread->getName(),threads_.size() );
  waitForFreeKMMLock();
  if ( threads_.size() > 1 )
  {
    Thread *tmp_thread;
    for ( uint32 c=0; c< threads_.size(); ++c )
    {
      tmp_thread = threads_.back();
      if ( tmp_thread == currentThread )
      {
        threads_.popBack();
        break;
      }
      threads_.rotateFront();
    }
  }
  unlockScheduling();
}

void Scheduler::sleep()
{
  currentThread->state_=Sleeping;
  //if we somehow stupidly go to sleep, block is automatically removed
  //we might break a lock in doing so, but that's still better than having no chance
  //of recovery whatsoever.
  unlockScheduling();
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
    ArchInterrupts::enableInterrupts();
    assert ( block_scheduling_extern_==0 );
    yield();
  }
}

void Scheduler::wake ( Thread* thread_to_wake )
{
  thread_to_wake->state_=Running;
}

void Scheduler::startThreadHack()
{
  currentThread->Run();
}

uint32 Scheduler::schedule()
{
  if ( testLock() || block_scheduling_extern_>0 )
  {
    //no scheduling today...
    //keep currentThread as it was
    //and stay in Kernel Kontext
    debug ( SCHEDULER,"schedule: currently blocked\n" );
    return 0;
  }

  do
  {
    // WARNING: do not read currentThread before is has been set here
    //          the first time scheduler is called.
    //          before this, currentThread may be 0 !!
    currentThread = threads_.front();

    if ( kill_old_ == false && currentThread->state_ == ToBeDestroyed )
      kill_old_=true;

    //this operation doesn't allocate or delete any kernel memory
    threads_.rotateBack();

  }
  while ( currentThread->state_ != Running );
  debug ( SCHEDULER,"Scheduler::schedule: new currentThread is %x %s, switch_userspace:%d\n",currentThread,currentThread->getName(),currentThread->switch_to_userspace_ );

  uint32 ret = 1;

  if ( currentThread->switch_to_userspace_ )
    currentThreadInfo =  currentThread->user_arch_thread_info_;
  else
  {
    currentThreadInfo =  currentThread->kernel_arch_thread_info_;
    ret=0;
  }

  return ret;
}

void Scheduler::yield()
{
  if ( ! ArchInterrupts::testIFSet() )
  {
    kprintfd ( "Scheduler::yield: WARNING Interrupts disabled, do you really want to yield ? (currentThread %x %s)\n", currentThread, currentThread->name_ );
    kprintf ( "Scheduler::yield: WARNING Interrupts disabled, do you really want to yield ?\n" );
  }
  ArchThreads::yield();
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
  //check outside of atmoarity for performance gain,
  //worst case, dead threads are around a while longer
  //then make sure we're atomar (can't really lock list, can I ;->)
  //NOTE: currentThread is always last on list

  if ( !kill_old_ )
    return;

  lockScheduling();
  waitForFreeKMMLock();
  List<Thread*> destroy_list;
  debug ( SCHEDULER,"cleanupDeadThreads: now running\n" );
  if ( kill_old_ )
  {
    Thread *tmp_thread;
    for ( uint32 c=0; c< threads_.size(); ++c )
    {
      tmp_thread = threads_.front();
      if ( tmp_thread->state_ == ToBeDestroyed )
      {
        destroy_list.pushBack ( tmp_thread );
        threads_.popFront();
        continue;
      }
      threads_.rotateBack();
    }
    kill_old_=false;
  }
  debug ( SCHEDULER, "cleanupDeadThreads: done\n" );
  unlockScheduling();
  while ( ! destroy_list.empty() )
  {
    Thread *cur_thread = destroy_list.front();
    destroy_list.popFront();
    delete cur_thread;
  }
}

void Scheduler::printThreadList()
{
  char *thread_states[6]= {"Running", "Sleeping", "ToBeDestroyed", "Unknown", "Unknown", "Unknown"};
  uint32 c=0;
  lockScheduling();
  debug ( SCHEDULER, "Scheduler::printThreadList: %d Threads in List\n",threads_.size() );
  for ( c=0; c<threads_.size();++c )
    debug ( SCHEDULER, "Scheduler::printThreadList: threads_[%d]: %x %s     [%s]\n",c,threads_[c],threads_[c]->getName(),thread_states[threads_[c]->state_] );
  unlockScheduling();
}

void Scheduler::lockScheduling()  //not as severe as stopping Interrupts
{
  if ( unlikely ( ArchThreads::testSetLock ( block_scheduling_,1 ) ) )
    arch_panic ( ( uint8* ) "FATAL ERROR: Scheduler::*: block_scheduling_ was set !! How the Hell did the program flow get here then ?\n" );
}
void Scheduler::unlockScheduling()
{
  block_scheduling_ = 0;
}
bool Scheduler::testLock()
{
  return ( block_scheduling_ > 0 );
}

void Scheduler::waitForFreeKMMLock()  //not as severe as stopping Interrupts
{
  if ( block_scheduling_==0 )
    arch_panic ( ( uint8* ) "FATAL ERROR: Scheduler::waitForFreeKMMLock: This is meant to be used while Scheduler is locked\n" );
  while ( ! KernelMemoryManager::instance()->isKMMLockFree() )
  {
    unlockScheduling();
    yield();
    lockScheduling();
  }
}

void Scheduler::disableScheduling()
{
  lockScheduling();
  block_scheduling_extern_++;
  unlockScheduling();
}
void Scheduler::reenableScheduling()
{
  lockScheduling();
  if ( block_scheduling_extern_>0 )
    block_scheduling_extern_--;
  unlockScheduling();
}
bool Scheduler::isSchedulingEnabled()
{
  if ( this )
    return ( block_scheduling_==0 && block_scheduling_extern_==0 );
  else
    return false;
}
