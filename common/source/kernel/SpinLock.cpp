//----------------------------------------------------------------------
//  $Id: SpinLock.cpp,v 1.5 2005/10/24 21:28:04 nelles Exp $
//----------------------------------------------------------------------
//
//  $Log: SpinLock.cpp,v $
//  Revision 1.4  2005/09/16 00:54:13  btittelbach
//  Small not-so-good Sync-Fix that works before Total-Syncstructure-Rewrite
//
//  Revision 1.3  2005/09/13 22:15:52  btittelbach
//  small BugFix: Scheduler really works now
//
//  Revision 1.2  2005/09/13 15:00:51  btittelbach
//  Prepare to be Synchronised...
//  kprintf_nosleep works now
//  scheduler/list still needs to be fixed
//
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
    if (unlikely(ArchInterrupts::testIFSet()==false))
      kpanict((uint8*)"SpinLock::acquire: with IF=0 ! Now we're dead !!!\nMaybe you used new/delete in irq/int-Handler context ?\n");
    //SpinLock: Simplest of Locks, do the next best thing to busy wating
    Scheduler::instance()->yield();
  }
}

bool SpinLock::isFree()
{
	return (nosleep_mutex_ == 0);
}

void SpinLock::release()
{
  nosleep_mutex_ = 0;
}
