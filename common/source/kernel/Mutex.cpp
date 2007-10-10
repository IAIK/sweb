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

Mutex::Mutex()
{
  mutex_ = 0;
}

void Mutex::acquire()
{
  spinlock_.acquire();
  while ( ArchThreads::testSetLock ( mutex_,1 ) )
  {
    sleepers_.pushBack ( currentThread );
    Scheduler::instance()->sleepAndRelease ( spinlock_ );
    spinlock_.acquire();
  }
  spinlock_.release();
  held_by_=currentThread;
}

void Mutex::release()
{
  spinlock_.acquire();
  mutex_ = 0;
  held_by_=0;
  if ( ! sleepers_.empty() )
  {
    Thread *thread = sleepers_.front();
    sleepers_.popFront();
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
