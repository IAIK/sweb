/**
 * @file SpinLock.cpp
 */

#include "SpinLock.h"
#include "console/kprintf.h"
#include "ArchThreads.h"
#include "ArchInterrupts.h"
#include "panic.h"
#include "Scheduler.h"
#include "Thread.h"

SpinLock::SpinLock() :
  nosleep_mutex_(0), held_by_(0)
{
}

bool SpinLock::acquireNonBlocking()
{
  if ( unlikely ( ArchInterrupts::testIFSet() ==false || !Scheduler::instance()->isSchedulingEnabled()))
  {
    kprintfd("(WARNING) SpinLock::acquireNonBlocking: with IF=%d and SchedulingEnabled=%d !\n", ArchInterrupts::testIFSet(), Scheduler::instance()->isSchedulingEnabled());
  }

  if ( ArchThreads::testSetLock ( nosleep_mutex_,1 ) )
  {
    return false;
  }
  held_by_ = currentThread;
  return true;
}

void SpinLock::acquire()
{
  if ( unlikely ( ArchInterrupts::testIFSet() ==false || !Scheduler::instance()->isSchedulingEnabled()))
  {
    kprintfd("(ERROR) SpinLock::acquire: with IF=%d and SchedulingEnabled=%d ! Now we're dead !!!\nMaybe you used new/delete in irq/int-Handler context or while Scheduling disabled?\n", ArchInterrupts::testIFSet(), Scheduler::instance()->isSchedulingEnabled());
    assert(false);
  }
  while ( ArchThreads::testSetLock ( nosleep_mutex_,1 ) )
  {
    //SpinLock: Simplest of Locks, do the next best thing to busy wating
    Scheduler::instance()->yield();
  }
  assert(held_by_ == 0);
  held_by_ = currentThread;
}

bool SpinLock::isFree()
{
  return ( nosleep_mutex_ == 0 );
}

void SpinLock::release()
{
  held_by_ = 0;
  nosleep_mutex_ = 0;
}
