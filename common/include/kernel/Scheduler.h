//----------------------------------------------------------------------
//   $Id: Scheduler.h,v 1.4 2005/06/14 18:51:47 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: Scheduler.h,v $
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
  Thread *Scheduler::xchangeThread(Thread *pop_up_thread);


  void yield();

  // NEVER EVER EVER CALL THIS ONE OUTSIDE OF AN INTERRUPT CONTEXT //
  uint32 schedule(uint32 from_interrupt=false);

private:

  Scheduler();

  static Scheduler *instance_;

  List<Thread*> threads_;
  //Thread* threads_[MAX_THREADS];
  
  static void startThreadHack();

};









#endif
