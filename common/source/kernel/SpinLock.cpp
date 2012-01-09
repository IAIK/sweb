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
  if ( ArchThreads::testSetLock ( nosleep_mutex_,1 ) )
  {
    if ( unlikely ( ArchInterrupts::testIFSet() ==false ) )
      kpanict ( ( uint8* ) "SpinLock::acquireNonBlocking: with IF=0 ! Now we're dead !!!\nMaybe you used new/delete in irq/int-Handler context ?\n" );
    return false;
  }
  held_by_ = currentThread;
  return true;
}

void SpinLock::acquire()
{
  while ( ArchThreads::testSetLock ( nosleep_mutex_,1 ) )
  {
    if ( unlikely ( ArchInterrupts::testIFSet() ==false ) )
      kpanict ( ( uint8* ) "SpinLock::acquire: with IF=0 ! Now we're dead !!!\nMaybe you used new/delete in irq/int-Handler context ?\n" );
    //SpinLock: Simplest of Locks, do the next best thing to busy wating
    Scheduler::instance()->yield();
  }
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
