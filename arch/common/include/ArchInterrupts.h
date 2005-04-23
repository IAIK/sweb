//----------------------------------------------------------------------
//  $Id: ArchInterrupts.h,v 1.1 2005/04/23 20:08:26 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: $
//----------------------------------------------------------------------


#ifndef _ARCH_INTERRUPTS_H_
#define _ARCH_INTERRUPTS_H_

#include "types.h"

class ArchInterrupts
{
public:
  
  // the timer handler callback will return 0 if nothing happened
  // or 1 if it wants to have a task switch
  // details of the task switch have to be set through other methods
  typedef uint32 (*TimerHandlerCallback)(void);

  // gives the arch a chance to set things up the way it wants to
  static void initialise();
  
  static void setTimerHandler(TimerHandlerCallback* timer_handler);

  static void enableTimer();
  static void disableTimer();
  
};









#endif
