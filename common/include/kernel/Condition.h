//----------------------------------------------------------------------
//   $Id: Condition.h,v 1.1 2005/09/07 00:33:52 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: Condition.h,v $
//
//----------------------------------------------------------------------

#ifndef _CONDITION_
#define _CONDITION_

#include "List.h"
#include "Thread.h"
#include "Mutex.h"

class Condition
{
  public:
  Condition(Mutex *lock);
  ~Condition();
  
  void wait();
  void signal();
  void broadcast();
  
  private:
  List<Thread *> *sleepers_;
  Mutex *lock_;
  //why do we need this Mutex* ?
  //because the only sane and working way to protect the list in the CV
  //is with the very same lock, the threads using the CV use.
  //extra lock inside the CV won't work -> deadlock possibility
  //mixing lock and switching of interrupts wont work -> irq during time I have lock
  //only way: interrupts off or same lock
  //and interrupts off we want to avoid
};

#endif
