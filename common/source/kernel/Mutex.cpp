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

Mutex::Mutex(const char* name) :
  mutex_(0),
  sleepers_(),
  held_by_(0),
  spinlock_(),
  name_(name ? name : "")
{
}

bool Mutex::acquireNonBlocking(const char* debug_info)
{
  if (held_by_ == currentThread && currentThread != 0)
  {
    kprintfd("Mutex::acquire: Deadlock: Mutex %s (%x) already held by currentThread (%x)\n", name_, this, currentThread);
    if (debug_info)
      kprintfd("Mutex::acquire: Debug Info: %s\n", debug_info);
    assert(false);
  }
  if(!spinlock_.acquireNonBlocking())
      return false;

  while ( ArchThreads::testSetLock ( mutex_,1 ) )
  {
    if(threadOnList(currentThread))
    {
      kprintfd ( "Mutex::acquire: thread %s going to sleep is already on sleepers-list of mutex %s (%x)\n"
                 "you shouldn't use Scheduler::wake() with a thread sleeping on a mutex\n", currentThread->getName(), name_, this );
      if (debug_info)
        kprintfd("Mutex::acquire: Debug Info: %s\n", debug_info);
      assert(false);
    }

    spinlock_.release();
    return false;
  }
  spinlock_.release();
  held_by_=currentThread;
  return true;
}

void Mutex::acquire(const char* debug_info)
{
  if (held_by_ == currentThread && currentThread != 0)
  {
    kprintfd("Mutex::acquire: Deadlock: Mutex %s (%x) already held by currentThread (%x)\n", name_, this, currentThread);
    if (debug_info)
      kprintfd("Mutex::acquire: Debug Info: %s\n", debug_info);
    assert(false);
  }
  spinlock_.acquire();
  while ( ArchThreads::testSetLock ( mutex_,1 ) )
  {
    if(threadOnList(currentThread))
    {
      kprintfd ( "Mutex::acquire: thread %s going to sleep is already on sleepers-list of mutex %s (%x)\n"
                 "you shouldn't use Scheduler::wake() with a thread sleeping on a mutex\n", currentThread->getName(), name_, this );
      if (debug_info)
        kprintfd("Mutex::acquire: Debug Info: %s\n", debug_info);
      assert(false);
    }

    sleepers_.push_back ( currentThread );
    Scheduler::instance()->sleepAndRelease ( spinlock_ );
    spinlock_.acquire();
  }
  spinlock_.release();
  held_by_=currentThread;
}

void Mutex::release()
{
  if (held_by_ != currentThread) // this is a mutex - not a binary semaphore!
  { // ... and yes - I'm pretty sure, we are can safely do this without the spinlock.

    kprintfd("\n\nMutex::release(): Mutex %s (%x) currently not held by currentThread!\n"
      "held_by <%s (%x)> currentThread <%s (%x)>\n\n\n", name_, this,
     (held_by_ ? held_by_->getName() : "(NULL)"), held_by_, currentThread->getName(), currentThread);

    kprintf("\n\nMutex::release(): Mutex %s (%x) currently not held by currentThread!\n"
      "held_by <%s (%x)> currentThread <%s (%x)>\n\n\n", name_, this,
     (held_by_ ? held_by_->getName() : "(NULL)"), held_by_, currentThread->getName(), currentThread);

    return;
  }

  spinlock_.acquire();
  mutex_ = 0;
  held_by_=0;
  if ( ! sleepers_.empty() )
  {
    Thread *thread = sleepers_.front();
    sleepers_.pop_front();
    Scheduler::instance()->wake ( thread );
  }
  spinlock_.release();
}

bool Mutex::isFree()
{
  if ( unlikely ( ArchInterrupts::testIFSet() && Scheduler::instance()->isSchedulingEnabled() ) )
    kpanict ( ( uint8* ) ( "Mutex::isFree: ERROR: Should not be used with IF=1 AND enabled Scheduler, use acquire instead\n" ) );

  if ( !spinlock_.isFree() || mutex_ > 0 )
    return false;
  else
    return true;
}

bool Mutex::threadOnList(Thread *thread)
{
  bool return_value = false;
  for (uint32 i=0; i < sleepers_.size(); i++  )
  {
    if(thread == sleepers_.front())
      return_value = true;
    ustl::rotate(sleepers_.begin(),sleepers_.begin()+1, sleepers_.end());
  }
  return return_value;
}
