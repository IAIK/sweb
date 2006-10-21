//----------------------------------------------------------------------
//   $Id: Scheduler.cpp,v 1.38 2006/10/21 23:32:04 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: Scheduler.cpp,v $
//  Revision 1.37  2006/10/13 11:38:12  btittelbach
//  Ein Bissal Uebersichtlichkeit im Bochs Terminal (aka loopende kprintfs auskomentiert)
//
//  Revision 1.36  2005/10/27 21:42:51  btittelbach
//  -Mutex::isFree() abuse check kennt jetzt auch Scheduler-Ausschalten und springt nicht mehr versehentlich an
//  -im Scheduler möglichen null-pointer zugriff vermieden
//  -kprintf nosleep check logik optimiert
//
//  Revision 1.35  2005/10/26 11:17:40  btittelbach
//  -fixed KMM/SchedulerBlock Deadlock
//  -introduced possible dangeours reenable-/disable-Scheduler Methods
//  -discovered that removing the IF/Lock Checks in kprintfd_nosleep is a VERY BAD Idea
//
//  Revision 1.34  2005/10/22 14:00:22  btittelbach
//  added sleepAndRelease()
//
//  Revision 1.33  2005/09/26 14:00:43  btittelbach
//  compilefix
//
//  Revision 1.32  2005/09/26 13:56:55  btittelbach
//  +doxyfication
//  +SchedulerClass upgrade
//
//  Revision 1.31  2005/09/20 14:32:08  btittelbach
//  better yet
//
//  Revision 1.30  2005/09/20 08:05:08  btittelbach
//  +kprintf flush fix: even though it worked fine before, now it works fine in theory as well ;->
//  +Condition cleanup
//  +FiFoDRBOSS now obsolete and removed
//  +added disk.img that nelle forgot to check in
//
//  Revision 1.29  2005/09/18 20:25:05  nelles
//
//
//  Block devices update.
//  See BDRequest and BDManager on how to use this.
//  Currently ATADriver is functional. The driver tries to detect if IRQ
//  mode is available and adjusts the mode of operation. Currently PIO
//  modes with IRQ or without it are supported.
//
//  TODO:
//  - add block PIO mode to read or write multiple sectors within one IRQ
//  - add DMA and UDMA mode :)
//
//
//   Committing in .
//
//   Modified Files:
//   	arch/common/include/ArchInterrupts.h
//   	arch/x86/source/ArchInterrupts.cpp
//   	arch/x86/source/InterruptUtils.cpp
//   	common/include/kernel/TestingThreads.h
//   	common/source/kernel/Makefile
//   	common/source/kernel/Scheduler.cpp
//   	common/source/kernel/main.cpp utils/bochs/bochsrc
//   Added Files:
//   	arch/x86/include/arch_bd_ata_driver.h
//   	arch/x86/include/arch_bd_driver.h
//   	arch/x86/include/arch_bd_ide_driver.h
//   	arch/x86/include/arch_bd_io.h
//  	arch/x86/include/arch_bd_manager.h
//   	arch/x86/include/arch_bd_request.h
//   	arch/x86/include/arch_bd_virtual_device.h
//   	arch/x86/source/arch_bd_ata_driver.cpp
//   	arch/x86/source/arch_bd_ide_driver.cpp
//   	arch/x86/source/arch_bd_manager.cpp
//  	arch/x86/source/arch_bd_virtual_device.cpp
//
//  Revision 1.28  2005/09/16 15:47:41  btittelbach
//  +even more KeyboardInput Bugfixes
//  +intruducing: kprint_buffer(..) (console write should never be used directly from anything with IF=0)
//  +Thread now remembers its Terminal
//  +Syscalls are USEABLE !! :-) IF=1 !!
//  +Syscalls can block now ! ;-) Waiting for Input...
//  +more other Bugfixes
//
//  Revision 1.27  2005/09/15 18:47:07  btittelbach
//  FiFoDRBOSS should only be used in interruptHandler Kontext, for everything else use FiFo
//  IdleThread now uses hlt instead of yield.
//
//  Revision 1.26  2005/09/15 17:51:13  nelles
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
//  Revision 1.25  2005/09/13 22:15:52  btittelbach
//  small BugFix: Scheduler really works now
//
//  Revision 1.24  2005/09/13 21:24:42  btittelbach
//  Scheduler without Memory Allocation in critical context (at least in Theory)
//
//  Revision 1.21  2005/09/07 00:33:52  btittelbach
//  +More Bugfixes
//  +Character Queue (FiFoDRBOSS) from irq with Synchronisation that actually works
//
//  Revision 1.20  2005/09/06 09:56:50  btittelbach
//  +Thread Names
//  +stdin Test Example
//
//  Revision 1.19  2005/09/05 23:01:24  btittelbach
//  Keyboard Input Handler
//  + several Bugfixes
//
//  Revision 1.18  2005/08/26 12:01:25  nomenquis
//  pagefaults in userspace now should really really really work
//
//  Revision 1.17  2005/08/11 16:18:02  nomenquis
//  fixed a bug
//
//  Revision 1.16  2005/08/07 16:47:25  btittelbach
//  More nice synchronisation Experiments..
//  RaceCondition/kprintf_nosleep related ?/infinite memory write loop Error still not found
//  kprintfd doesn't use a buffer anymore, as output_bochs blocks anyhow, should propably use some arch-specific interface instead
//
//  Revision 1.15  2005/08/04 20:47:43  btittelbach
//  Where is the Bug, maybe I will see something tomorrow that I didn't see today
//
//  Revision 1.14  2005/07/27 10:04:26  btittelbach
//  kprintf_nosleep and kprintfd_nosleep now works
//  Output happens in dedicated Thread using VERY EVIL Mutex Hack
//
//  Revision 1.13  2005/07/24 17:02:59  nomenquis
//  lots of changes for new console stuff
//
//  Revision 1.12  2005/07/21 19:08:41  btittelbach
//  Jö schön, Threads u. Userprozesse werden ordnungsgemäß beendet
//  Threads können schlafen, Mutex benutzt das jetzt auch
//  Jetzt muß nur der Mutex auch überall verwendet werden
//
//  Revision 1.11  2005/07/12 21:05:38  btittelbach
//  Lustiges Spielen mit UserProgramm Terminierung
//
//  Revision 1.10  2005/07/05 20:22:56  btittelbach
//  some changes
//
//  Revision 1.9  2005/07/05 17:29:48  btittelbach
//  new kprintf(d) Policy:
//  [Class::]Function: before start of debug message
//  Function can be abbreviated "ctor" if Constructor
//  use kprintfd where possible
//
//  Revision 1.8  2005/06/14 18:51:47  btittelbach
//  afterthought page fault handling
//
//  Revision 1.7  2005/05/31 18:13:14  nomenquis
//  fixed compile errors
//
//  Revision 1.6  2005/05/31 17:29:16  nomenquis
//  userspace
//
//  Revision 1.5  2005/05/31 17:25:56  btittelbach
//  Scheduler mit Listen geschmückt
//
//  Revision 1.4  2005/05/25 08:27:49  nomenquis
//  cr3 remapping finally really works now
//
//  Revision 1.3  2005/05/08 21:43:55  nelles
//  changed gcc flags from -g to -g3 -gstabs in order to
//  generate stabs output in object files
//  changed linker script to load stabs in kernel
//  in bss area so GRUB loads them automaticaly with
//  the bss section
//
//  changed StupidThreads in main for testing purposes
//
//  Revision 1.2  2005/04/27 08:58:16  nomenquis
//  locks work!
//  w00t !
//
//  Revision 1.1  2005/04/26 15:58:45  nomenquis
//  threads, scheduler, happy day
//
//----------------------------------------------------------------------


#include "Scheduler.h"
#include "Thread.h"
#include "arch_panic.h"
#include "ArchThreads.h"
#include "console/kprintf.h"
#include "ArchInterrupts.h"
#include "mm/KernelMemoryManager.h"

ArchThreadInfo *currentThreadInfo;
Thread *currentThread;



Scheduler *Scheduler::instance_=0;

Scheduler *Scheduler::instance()
{
  return instance_;
}

class IdleThread : public Thread
{
public:
  
  IdleThread()
  {
    name_="IdleThread";
  }

  virtual void Run()
  {
    while (1)
    {
      Scheduler::instance()->cleanupDeadThreads();
      //Scheduler::instance()->yield();
      __asm__ __volatile__("hlt");
    }
  }
};

void Scheduler::createScheduler()
{
  instance_ = new Scheduler();
  
  // create idle thread, this one really does not do too much

  Thread *idle = new IdleThread();
  instance_->addNewThread(idle);
}

Scheduler::Scheduler()
{
  kill_old_=false;
  block_scheduling_=0;
  block_scheduling_extern_=0;
}

void Scheduler::addNewThread(Thread *thread)
{
  //new Thread gets scheduled next
  //also gets added to front as not to interfere with remove or xchange

  lockScheduling();
  //kprintfd_nosleep("Scheduler::addNewThread: %x %s\n",thread,thread->getName());
  waitForFreeKMMLock();
  threads_.pushFront(thread);
  unlockScheduling();
}


//you can't remove the last thread
void Scheduler::removeCurrentThread()
{
  lockScheduling();
  //kprintfd_nosleep("Scheduler::removeCurrentThread: %x %s, threads_.size() %d\n",currentThread,currentThread->getName(),threads_.size());
  waitForFreeKMMLock();
  if (threads_.size() > 1)
  {
    Thread *tmp_thread;
    for (uint32 c=0; c< threads_.size(); ++c)
    {
      tmp_thread = threads_.back();
      if (tmp_thread == currentThread)
	  {
			threads_.popBack();
			break;
	  }
      threads_.rotateFront();
    }
  }
  unlockScheduling();
}

void Scheduler::sleep()
{
  currentThread->state_=Sleeping;
  //if we somehow stupidly go to sleep, block is automatically removed
  //we might break a lock in doing so, but that's still better than having no chance
  //of recovery whatsoever.
  unlockScheduling();
  yield();
}

void Scheduler::sleepAndRelease(SpinLock &lock)
{
  lockScheduling();
  currentThread->state_=Sleeping;
  lock.release();
  unlockScheduling();
  yield();
}
void Scheduler::sleepAndRelease(Mutex &lock)
{
  lockScheduling();
  currentThread->state_=Sleeping;
  lock.release();
  unlockScheduling();
  yield();
}
void Scheduler::wake(Thread* thread_to_wake)
{
  //DEBUG: Check if thread_to_wake is in List
  //if (checkThreadExists(thread_to_wake)
  thread_to_wake->state_=Running;
}

void Scheduler::startThreadHack()
{
  currentThread->Run();
}

uint32 Scheduler::schedule()
{
  assert(this);
  if (testLock() || block_scheduling_extern_>0)
  {
    //no scheduling today...
    //keep currentThread as it was
    //and stay in Kernel Kontext
  //  kprintfd_nosleep("Scheduler::schedule: currently blocked\n");
    return 0;
  }

  do 
  {
    currentThread = threads_.front();
    
    if (kill_old_ == false && currentThread->state_ == ToBeDestroyed)
      kill_old_=true;
    
    //this operation doesn't allocate or delete any kernel memory
    threads_.rotateBack();
    
  } while (currentThread->state_ != Running);
  //kprintfd_nosleep("Scheduler::schedule: new currentThread is %x %s, switch_userspace:%d\n",currentThread,currentThread->getName(),currentThread->switch_to_userspace_);
  
  uint32 ret = 1;
  
  if ( currentThread->switch_to_userspace_)
    currentThreadInfo =  currentThread->user_arch_thread_info_;
  else
  {
    currentThreadInfo =  currentThread->kernel_arch_thread_info_;
    ret=0;
  }
  
  return ret;
}

void Scheduler::yield()
{
  if (! ArchInterrupts::testIFSet())
  {
    kprintfd("Scheduler::yield: WARNING Interrupts disabled, do you really want to yield ? (currentThread %x %s)\n", currentThread, currentThread->name_);
    kprintf("Scheduler::yield: WARNING Interrupts disabled, do you really want to yield ?\n");
  }
  ArchThreads::yield();
}

bool Scheduler::checkThreadExists(Thread* thread)
{
  bool retval=false;
  lockScheduling();
  for (uint32 c=0; c<threads_.size();++c) //fortunately this doesn't involve KMM
    if (threads_[c]==thread)
    {
      retval=true;
      break;
    }
  unlockScheduling();
  return retval;
}

void Scheduler::cleanupDeadThreads()
{
  //check outside of atmoarity for performance gain,
  // worst case, dead threads are around a while longer
  //then make sure we're atomar (can't really lock list, can I ;->)
  //note: currentThread is always last on list

  if (!kill_old_)
    return;
  
  lockScheduling();
  waitForFreeKMMLock();
  //kprintfd_nosleep("Scheduler::cleanupDeadThreads: now running\n");
  if (kill_old_)
  {
    Thread *tmp_thread;
    for (uint32 c=0; c< threads_.size(); ++c)
    {
      tmp_thread = threads_.front();
      if (tmp_thread->state_ == ToBeDestroyed)      
      {
        delete tmp_thread;
        threads_.popFront();
        continue;
      }
      threads_.rotateBack();
    }
    kill_old_=false;
  }
  //kprintfd_nosleep("Scheduler::cleanupDeadThreads: done\n");
  unlockScheduling();
}

void Scheduler::printThreadList()
{
  char *thread_states[6]= {"Running", "Sleeping", "ToBeDestroyed", "Unknown", "Unknown", "Unknown"};
  uint32 c=0;
  lockScheduling();
  kprintfd_nosleep("Scheduler::printThreadList: %d Threads in List\n",threads_.size());
  for (c=0; c<threads_.size();++c)
    kprintfd_nosleep("Scheduler::printThreadList: threads_[%d]: %x %s     [%s]\n",c,threads_[c],threads_[c]->getName(),thread_states[threads_[c]->state_]);
  unlockScheduling();
}

void Scheduler::lockScheduling()  //not as severe as stopping Interrupts
{
  if (unlikely(ArchThreads::testSetLock(block_scheduling_,1)))
    arch_panic((uint8*) "FATAL ERROR: Scheduler::*: block_scheduling_ was set !! How the Hell did the program flow get here then ?\n");  
}
void Scheduler::unlockScheduling()
{
  block_scheduling_ = 0;
}
bool Scheduler::testLock() {
  return (block_scheduling_ > 0);
}

void Scheduler::waitForFreeKMMLock()  //not as severe as stopping Interrupts
{
  if (block_scheduling_==0)
    arch_panic((uint8*) "FATAL ERROR: Scheduler::waitForFreeKMMLock: This is meant to be used while Scheduler is locked\n");  
  while (! KernelMemoryManager::instance()->isKMMLockFree())
  {
    unlockScheduling();
    yield();
    lockScheduling();
  }
}

void Scheduler::disableScheduling()
{
  lockScheduling();
  block_scheduling_extern_++;
  unlockScheduling();
}
void Scheduler::reenableScheduling()
{
  lockScheduling();
  if (block_scheduling_extern_>0)
    block_scheduling_extern_--;
  unlockScheduling();  
}
bool Scheduler::isSchedulingEnabled()
{
  if (this)
    return (block_scheduling_==0 && block_scheduling_extern_==0);
  else
    return false;
}
