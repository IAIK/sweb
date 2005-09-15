//----------------------------------------------------------------------
//  $Id: Mutex.cpp,v 1.8 2005/09/15 17:51:13 nelles Exp $
//----------------------------------------------------------------------
//
//  $Log: Mutex.cpp,v $
//  Revision 1.7  2005/09/07 00:33:52  btittelbach
//  +More Bugfixes
//  +Character Queue (FiFoDRBOSS) from irq with Synchronisation that actually works
//
//  Revision 1.6  2005/08/07 16:47:25  btittelbach
//  More nice synchronisation Experiments..
//  RaceCondition/kprintf_nosleep related ?/infinite memory write loop Error still not found
//  kprintfd doesn't use a buffer anymore, as output_bochs blocks anyhow, should propably use some arch-specific interface instead
//
//  Revision 1.5  2005/07/24 17:02:59  nomenquis
//  lots of changes for new console stuff
//
//  Revision 1.4  2005/07/21 19:08:41  btittelbach
//  Jö schön, Threads u. Userprozesse werden ordnungsgemäß beendet
//  Threads können schlafen, Mutex benutzt das jetzt auch
//  Jetzt muß nur der Mutex auch überall verwendet werden
//
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
#include "ArchInterrupts.h"
#include "debug_bochs.h"
#include "Scheduler.h"
#include "Thread.h"
#include "arch_panic.h"

Mutex::Mutex()
{
  mutex_ = 0;
}

void Mutex::acquire()
{
  while (ArchThreads::testSetLock(mutex_,1))
  {
    if (unlikely (! ArchInterrupts::testIFSet()))
      writeLine2Bochs(reinterpret_cast<const uint8*>("Mutex::acquire: WARNING Interrupts were off (now switiching on)\n"));
    ArchInterrupts::disableInterrupts();
//    kprintfd("Mutex::Acquire: could not get lock, going to sleep()\n");
    sleepers_.pushBack(currentThread);
    ArchInterrupts::enableInterrupts();
    Scheduler::instance()->sleep();
//    kprintfd("Mutex::Acquire: Wakeup after yield()\n");
   
  }
  held_by_=currentThread;
}

void Mutex::release()
{
  mutex_ = 0;
  held_by_=0;
  if (! sleepers_.empty())
  {
    if (unlikely (! ArchInterrupts::testIFSet()))
      writeLine2Bochs(reinterpret_cast<const uint8*>("Mutex::acquire: WARNING Interrupts were off (now switiching on)\n"));
    ArchInterrupts::disableInterrupts();
    Thread *thread = sleepers_.front();
    sleepers_.popFront();
    ArchInterrupts::enableInterrupts();
    Scheduler::instance()->wake(thread);
  }
}

bool Mutex::isFree()
{
  if (unlikely (ArchInterrupts::testIFSet()))
    arch_panic((uint8*)("Mutex::isFree: ERROR: Should not be used with IF=1, use acquire instead\n"));

  if (mutex_ > 0)
    return false;
  else
    return true;
}

bool Mutex::isFreeAtomic()
{
  if (mutex_ > 0)
    return false;
  else
    return true;
}