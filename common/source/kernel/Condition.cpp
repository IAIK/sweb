//----------------------------------------------------------------------
//   $Id: Condition.cpp,v 1.6 2006/10/13 11:38:12 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: Condition.cpp,v $
//  Revision 1.5  2005/09/20 08:05:08  btittelbach
//  +kprintf flush fix: even though it worked fine before, now it works fine in theory as well ;->
//  +Condition cleanup
//  +FiFoDRBOSS now obsolete and removed
//  +added disk.img that nelle forgot to check in
//
//  Revision 1.4  2005/09/16 15:47:41  btittelbach
//  +even more KeyboardInput Bugfixes
//  +intruducing: kprint_buffer(..) (console write should never be used directly from anything with IF=0)
//  +Thread now remembers its Terminal
//  +Syscalls are USEABLE !! :-) IF=1 !!
//  +Syscalls can block now ! ;-) Waiting for Input...
//  +more other Bugfixes
//
//  Revision 1.3  2005/09/16 00:54:13  btittelbach
//  Small not-so-good Sync-Fix that works before Total-Syncstructure-Rewrite
//
//  Revision 1.2  2005/09/15 17:51:13  nelles
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
//  Revision 1.1  2005/09/07 00:33:52  btittelbach
//  +More Bugfixes
//  +Character Queue (FiFoDRBOSS) from irq with Synchronisation that actually works
//
//
//----------------------------------------------------------------------

#include "Condition.h"
#include "Scheduler.h"
#include "assert.h"
#include "console/kprintf.h"
#include "ArchInterrupts.h"

Condition::Condition(Mutex *lock)
{
  sleepers_=new List<Thread *>();
  lock_=lock;
}
Condition::~Condition()
{
  delete sleepers_;
}

void Condition::wait()
{
  // list is protected, because we assume, the lock is being held
  assert(lock_->isHeldBy(currentThread));
  assert(ArchInterrupts::testIFSet());
  sleepers_->pushBack(currentThread);
  lock_->release();
  //<-- an interrupt and signal could happen here or during "sleep()"  ! problem: Thread* gets deleted before thread goes to sleep -> no wakeup call possible on next signal
  //kprintfd("Condition::wait: Thread %x %s wating on Condition %x\n",currentThread,currentThread->getName(),this);
  Scheduler::instance()->sleep();
  lock_->acquire();
}

void Condition::signal()
{
  if (! lock_->isHeldBy(currentThread))
    return;
  assert(ArchInterrupts::testIFSet());
  Thread *thread=0;
  if (!sleepers_->empty())
  {
    thread = sleepers_->front();
    if (thread->state_ == Sleeping)
    {
      //Solution to above Problem: Wake and Remove from List only Threads which are actually sleeping
      Scheduler::instance()->wake(thread);
      sleepers_->popFront();
    }    
  }
  //if (thread)
    //kprintfd("Condition::signal: Thread %x %s being signaled for Condition %x\n",thread,thread->getName(),this);
}

void Condition::broadcast()
{
  if (! lock_->isHeldBy(currentThread))
    return;
  assert(ArchInterrupts::testIFSet());
  Thread *thread;
  List<Thread*> tmp_threads;
  while (!sleepers_->empty())
  {
    thread = sleepers_->front();
    sleepers_->popFront();
    if (thread->state_ == Sleeping)
      Scheduler::instance()->wake(thread);
    else
      tmp_threads.pushBack(thread);
    //kprintfd("Condition::broadcast: Thread %x %s being signaled for Condition %x\n",thread,thread->getName(),this);
  }
  while (!tmp_threads.empty())
  {
    sleepers_->pushBack(tmp_threads.front());
    tmp_threads.popFront();
  }
}
