//----------------------------------------------------------------------
//  $Id: SpinLock.cpp,v 1.1 2005/08/07 16:54:21 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: SpinLock.cpp,v $
//----------------------------------------------------------------------

#include "SpinLock.h"
#include "console/kprintf.h"
#include "ArchThreads.h"
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
    //SpinLock: Simplest of Locks, do the next best thing to busy wating
    Scheduler::instance()->yield();
  }
}

void SpinLock::release()
{
  nosleep_mutex_ = 0;
}
