//----------------------------------------------------------------------
//  $Id: ArchInterrupts.cpp,v 1.1 2005/04/23 20:08:26 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: $
//----------------------------------------------------------------------

#include "ArchInterrupts.h"

static uint32 interrupts_on = 0;
static uint32 timer_on = 0;

// gives the arch a chance to set things up the way it wants to
void ArchInterrupts::initialise()
{
  
}

void ArchInterrupts::setTimerHandler(TimerHandlerCallback* timer_handler)
{
  
}

void ArchInterrupts::enableTimer()
{
  
}

void ArchInterrupts::disableTimer()
{
  
  
}
