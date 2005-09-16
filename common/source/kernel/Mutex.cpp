//----------------------------------------------------------------------
//  $Id: Mutex.cpp,v 1.10 2005/09/16 00:54:13 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: Mutex.cpp,v $
//  Revision 1.9  2005/09/15 18:47:07  btittelbach
//  FiFoDRBOSS should only be used in interruptHandler Kontext, for everything else use FiFo
//  IdleThread now uses hlt instead of yield.
//
//  Revision 1.8  2005/09/15 17:51:13  nelles
//
//
//   Massive update. Like PatchThursday.
//   Keyboard is now available.
//   Each Terminal has a buffer attached to it and threads should read the buffer
//   of the attached terminal. See TestingThreads.h in common/include/kernel for
//   example of how to do it.
//   Switching of the terminals is done with the SHFT+F-keys. (CTRL+Fkeys gets
//   eaten by X on my machine and does not reach Bochs).
//   Lot of smaller modifications, to FiFo, Mutex etc.
//
//   Committing in .
//
//   Modified Files:
//   	arch/x86/source/InterruptUtils.cpp
//   	common/include/console/Console.h
//   	common/include/console/Terminal.h
//   	common/include/console/TextConsole.h common/include/ipc/FiFo.h
//   	common/include/ipc/FiFoDRBOSS.h common/include/kernel/Mutex.h
//   	common/source/console/Console.cpp
//   	common/source/console/Makefile
//   	common/source/console/Terminal.cpp
//   	common/source/console/TextConsole.cpp
//   	common/source/kernel/Condition.cpp
//   	common/source/kernel/Mutex.cpp
//   	common/source/kernel/Scheduler.cpp
//   	common/source/kernel/Thread.cpp common/source/kernel/main.cpp
//   Added Files:
//   	arch/x86/include/arch_keyboard_manager.h
//   	arch/x86/source/arch_keyboard_manager.cpp
//
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
    spinlock_.acquire();
    sleepers_.pushBack(currentThread);
    spinlock_.release();
    Scheduler::instance()->sleep();
//    kprintfd("Mutex::Acquire: Wakeup after yield()\n");
   
  }
  held_by_=currentThread;
}

void Mutex::release()
{
  mutex_ = 0;
  held_by_=0;
  spinlock_.acquire();
  if (! sleepers_.empty())
  {
    Thread *thread = sleepers_.front();
    sleepers_.popFront();
    Scheduler::instance()->wake(thread);
  }
  spinlock_.release();
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
