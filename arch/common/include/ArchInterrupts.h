//----------------------------------------------------------------------
//  $Id: ArchInterrupts.h,v 1.4 2005/04/25 21:15:41 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchInterrupts.h,v $
//  Revision 1.3  2005/04/24 16:58:03  nomenquis
//  ultra hack threading
//
//  Revision 1.2  2005/04/23 20:32:30  nomenquis
//  timer interrupt works
//
//  Revision 1.1  2005/04/23 20:08:26  nomenquis
//  updates
//
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
  typedef uint32 (*TimerHandlerCallback)();

  // gives the arch a chance to set things up the way it wants to
  static void initialise();

  static void enableTimer();
  static void disableTimer();
  
  // enable interrupts, no matter what, this is bad
  static void enableInterrupts();
  static uint32 disableInterrupts();
  static void setOldInterruptState(uint32 const &flags);

};









#endif
