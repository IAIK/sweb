/**
 * @file Mutex.cpp
 */

#include "Mutex.h"
#include "console/kprintf.h"
#include "ArchThreads.h"
#include "ArchInterrupts.h"
#include "debug_bochs.h"
#include "Scheduler.h"
#include "Thread.h"
#include "panic.h"

extern uint32 boot_completed;

Mutex::Mutex(const char* name) :
  mutex_(0),
  sleepers_(),
  held_by_(0),
  spinlock_(name),
  name_(name ? name : "")
{
}

Mutex::~Mutex()
{
  spinlock_.acquire();
  if (sleepers_.size() != 0)
    kprintfd("WARNING: Mutex::~Mutex %s (%x) with sleepers_.size() != 0 and currentThread (%x)\n", name_, this, currentThread);
  if (held_by_ != 0 && held_by_ != currentThread)
    kprintfd("WARNING: Mutex::~Mutex %s (%x) with held_by_ != 0 && held_by_ != currentThread and currentThread (%x) and held_by_ (%x)\n", name_, this, currentThread, held_by_);
  spinlock_.release();
}

bool Mutex::acquireNonBlocking(const char* debug_info)
{
  if (likely(boot_completed))
  {
    checkDeadlock("Mutex::acquireNonBlocking", debug_info);
    if (likely(boot_completed) && ArchThreads::testSetLock ( mutex_,1 ) )
    {
      if(threadOnList(currentThread))
      {
        boot_completed = 0;
        kprintfd ( "Mutex::acquire: thread %s going to sleep is already on sleepers-list of mutex %s (%x)\n"
                   "you shouldn't use Scheduler::wake() with a thread sleeping on a mutex\n", currentThread->getName(), name_, this );
        if (debug_info)
          kprintfd("Mutex::acquire: Debug Info: %s\n", debug_info);
        assert(false);
      }
      return false;
    }
    assert(held_by_ == 0);
    held_by_=currentThread;
  }
  return true;
}

void Mutex::acquire(const char* debug_info)
{
  if (likely(boot_completed))
  {
    //kprintfd("Mutex::acquire %x %s, %s\n", this, name_, debug_info);
    checkDeadlock("Mutex::acquire", debug_info);

    while ( ArchThreads::testSetLock ( mutex_,1 ) )
    {
      if(threadOnList(currentThread))
      {
        boot_completed = 0;
        kprintfd ( "Mutex::acquire: thread %s going to sleep is already on sleepers-list of mutex %s (%x)\n"
                   "you shouldn't use Scheduler::wake() with a thread sleeping on a mutex\n", currentThread->getName(), name_, this );
        if (debug_info)
          kprintfd("Mutex::acquire: Debug Info: %s\n", debug_info);
        assert(false);
      }

      checkCircularDeadlock("Mutex::acquire", debug_info, currentThread, false);
      spinlock_.acquire();
      sleepers_.push_back ( currentThread );
      currentThread->sleeping_on_mutex_ = this;
      Scheduler::instance()->sleepAndRelease(spinlock_);
      currentThread->sleeping_on_mutex_ = 0;
    }
    assert(held_by_ == 0);
    held_by_=currentThread;
  }
}

void Mutex::release(const char* debug_info)
{
  checkInvalidRelease("Mutex::release", debug_info);
  //kprintfd("Mutex::release %x %s, %s\n", this, name_, debug_info);
  held_by_=0;
  mutex_ = 0;
  spinlock_.acquire();
  if ( ! sleepers_.empty() )
  {
    Thread *thread = sleepers_.front();
    sleepers_.pop_front();
    spinlock_.release();
    Scheduler::instance()->wake ( thread );
  }
  else
    spinlock_.release();
}

bool Mutex::isFree()
{
  if ( unlikely ( ArchInterrupts::testIFSet() && Scheduler::instance()->isSchedulingEnabled() ) )
    kpanict ( ( uint8* ) ( "Mutex::isFree: ERROR: Should not be used with IF=1 AND enabled Scheduler, use acquire instead\n" ) );

  if ( mutex_ > 0 )
    return false;
  else
    return true;
}

bool Mutex::threadOnList(Thread *thread)
{
  spinlock_.acquire();
  bool result = !(ustl::find(sleepers_, thread) == sleepers_.end());
  spinlock_.release();
  return result;
}

void Mutex::checkDeadlock(const char* method, const char* debug_info)
{
  if (held_by_ == currentThread && currentThread != 0)
  {
    kprintfd("%s: Deadlock: Mutex %s (%x) already held by currentThread (%x)\n", name_, method, this, currentThread);
    if (debug_info)
      kprintfd("%s: Debug Info: %s\n", method, debug_info);
    assert(false);
  }
}

void Mutex::checkCircularDeadlock(const char* method, const char* debug_info, Thread* start, bool output)
{
  if (start == 0)
    return;

  if (held_by_ != 0 && held_by_ == start)
  {
    kprintfd("%s: WARNING - Potential Deadlock: when sleeping on Mutex %s (%x) with currentThread (%x)\n", name_, method, this, currentThread);
    if (!output)
      checkCircularDeadlock(method, debug_info, start, true);
    return;
  }

  if (held_by_ != 0 && held_by_->state_ == Sleeping)
  {
    if (held_by_->sleeping_on_mutex_ != 0)
    {
      if (output)
        kprintfd("              Potential Deadlock: Mutex %s (%x) held_by_ (%x)\n", name_, this, held_by_);
      held_by_->sleeping_on_mutex_->checkCircularDeadlock(method, debug_info, start, output);
    }
  }
}

void Mutex::checkInvalidRelease(const char* method, const char* debug_info)
{
  if (boot_completed && held_by_ != currentThread) // this is a mutex - not a binary semaphore!
  { // ... and yes - I'm pretty sure, we can safely do this without the spinlock.
    boot_completed = 0;

    kprintfd("\n\n%s: Mutex %s (%x) currently not held by currentThread!\n"
        "held_by <%s (%x)> currentThread <%s (%x)>\ndebug info: %s\n\n", method, name_, this,
        (held_by_ ? held_by_->getName() : "(NULL)"), held_by_, currentThread->getName(), currentThread, debug_info ? debug_info : "(NULL)");

    kprintf("\n\n%s: Mutex %s (%x) currently not held by currentThread!\n"
        "held_by <%s (%x)> currentThread <%s (%x)>\ndebug info: %s\n\n", method, name_, this,
        (held_by_ ? held_by_->getName() : "(NULL)"), held_by_, currentThread->getName(), currentThread, debug_info ? debug_info : "(NULL)");

    assert(false);
  }
}
