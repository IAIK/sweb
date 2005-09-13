//----------------------------------------------------------------------
//   $Id: Scheduler.h,v 1.12 2005/09/13 22:15:51 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: Scheduler.h,v $
//  Revision 1.11  2005/09/13 21:24:42  btittelbach
//  Scheduler without Memory Allocation in critical context (at least in Theory)
//
//  Revision 1.9  2005/09/07 00:33:52  btittelbach
//  +More Bugfixes
//  +Character Queue (FiFoDRBOSS) from irq with Synchronisation that actually works
//
//  Revision 1.8  2005/09/05 23:01:24  btittelbach
//  Keyboard Input Handler
//  + several Bugfixes
//
//  Revision 1.7  2005/08/07 16:47:24  btittelbach
//  More nice synchronisation Experiments..
//  RaceCondition/kprintf_nosleep related ?/infinite memory write loop Error still not found
//  kprintfd doesn't use a buffer anymore, as output_bochs blocks anyhow, should propably use some arch-specific interface instead
//
//  Revision 1.6  2005/07/21 19:08:41  btittelbach
//  Jö schön, Threads u. Userprozesse werden ordnungsgemäß beendet
//  Threads können schlafen, Mutex benutzt das jetzt auch
//  Jetzt muß nur der Mutex auch überall verwendet werden
//
//  Revision 1.5  2005/07/05 20:22:56  btittelbach
//  some changes
//
//  Revision 1.4  2005/06/14 18:51:47  btittelbach
//  afterthought page fault handling
//
//  Revision 1.3  2005/05/31 17:25:56  btittelbach
//  Scheduler mit Listen geschmückt
//
//  Revision 1.2  2005/05/25 08:27:48  nomenquis
//  cr3 remapping finally really works now
//
//  Revision 1.1  2005/04/26 15:58:45  nomenquis
//  threads, scheduler, happy day
//
//----------------------------------------------------------------------


#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "types.h"
#include "List.h"


#define MAX_THREADS 20

class Thread;
class ArchThreadInfo;
  
extern ArchThreadInfo *currentThreadInfo;
extern Thread *currentThread;

class Scheduler
{
public:
  
  static Scheduler *instance();
  static void createScheduler();

  void addNewThread(Thread *thread);
  void removeCurrentThread();
  void sleep();
  void wake(Thread* thread_to_wake);
  void cleanupDeadThreads();

  void yield();

  void printThreadList();
  bool checkThreadExists(Thread* thread);

  // NEVER EVER EVER CALL THIS ONE OUTSIDE OF AN INTERRUPT CONTEXT //
  uint32 schedule(uint32 from_interrupt=false);

private:

  Scheduler();

  //don't use this externally
  //this is only to protect the thread list
  void lockScheduling();  //not as severe as stopping Interrupts
  void unlockScheduling();
  bool testLock();

  static Scheduler *instance_;

  List<Thread*> threads_;

  static void startThreadHack();

  bool kill_old_;

  uint32 block_scheduling_;

};
#endif
