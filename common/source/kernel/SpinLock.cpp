//----------------------------------------------------------------------
//  $Id: SpinLock.cpp,v 1.2 2005/09/13 15:00:51 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: SpinLock.cpp,v $
//  Revision 1.1  2005/08/07 16:54:21  btittelbach
//  SpinLock++ with Yield instead of BusyWaiting
//
//----------------------------------------------------------------------

#include "SpinLock.h"
#include "console/kprintf.h"
#include "ArchThreads.h"
#include "ArchInterrupts.h"
#include "panic.h"
#include "Scheduler.h"
#include "Thread.h"

SpinLock::SpinLock()
{
  nosleep_mutex_ = 0;
}

void SpinLock::acquire()
{
  while (ArchThreads::testSetLock(nosleep_mutex_,1))
  {
    if (ArchInterrupts::testIFSet()==false)
      kpanict((uint8*)"SpinLock::acquire: with IF=0 ! Now we're dead !!!");
    //SpinLock: Simplest of Locks, do the next best thing to busy wating
    Scheduler::instance()->yield();
  }
}

void SpinLock::release()
{
  nosleep_mutex_ = 0;
}
