//----------------------------------------------------------------------
//  $Id: Mutex.cpp,v 1.3 2005/07/05 17:29:48 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: Mutex.cpp,v $
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

void Mutex::Acquire()
{
  while (ArchThreads::testSetLock(mutex_,1))
  {
    kprintfd("Mutex::Acquire: could not get lock, going to yield()\n");
    Scheduler::instance()->yield();
    kprintfd("Mutex::Acquire: Wakeup after yield()\n");
   
  }
}

void Mutex::Release()
{
  mutex_ = 0;
  Scheduler::instance()->yield();
}
