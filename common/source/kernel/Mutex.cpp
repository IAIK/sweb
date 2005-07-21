//----------------------------------------------------------------------
//  $Id: Mutex.cpp,v 1.4 2005/07/21 19:08:41 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: Mutex.cpp,v $
//  Revision 1.3  2005/07/05 17:29:48  btittelbach
//  new kprintf(d) Policy:
//  [Class::]Function: before start of debug message
//  Function can be abbreviated "ctor" if Constructor
//  use kprintfd where possible
//
//  Revision 1.2  2005/04/27 08:58:16  nomenquis
//  locks work!
//  w00t !
//
//  Revision 1.1  2005/04/24 20:39:31  nomenquis
//  cleanups
//
//----------------------------------------------------------------------

#include "Mutex.h"
#include "console/kprintf.h"
#include "ArchThreads.h"
#include "Scheduler.h"
#include "Thread.h"

Mutex::Mutex()
{
  kprintfd("Mutex::Mutex");
  mutex_ = 0;
}

void Mutex::acquire()
{
  while (ArchThreads::testSetLock(mutex_,1))
  {
    kprintfd("Mutex::Acquire: could not get lock, going to sleep()\n");
    sleepers_.pushBack(currentThread);
    Scheduler::instance()->sleep();
    kprintfd("Mutex::Acquire: Wakeup after yield()\n");
   
  }
}

void Mutex::release()
{
  mutex_ = 0;
  if (! sleepers_.empty())
  {
    Scheduler::instance()->wake(sleepers_.front());
    sleepers_.popFront();
  }
}
