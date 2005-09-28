//----------------------------------------------------------------------
//  $Id: ArchInterrupts.cpp,v 1.4 2005/09/28 15:57:31 rotho Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchInterrupts.cpp,v $
//  Revision 1.3  2005/09/23 16:06:07  rotho
//  did some cleaning up
//
//  Revision 1.2  2005/09/21 02:18:58  rotho
//  temporary commit; still doesn't work
//
//  Revision 1.1  2005/08/01 08:18:59  nightcreature
//  initial release, partly dummy implementation, needs changes
//
//
//----------------------------------------------------------------------

#include "ArchInterrupts.h"
#include "ports.h"
extern "C" 
{
#include "os.h"
}
#include "hypervisor.h"

#include "events.h"

static uint32 interrupts_on = 0;
static uint32 timer_on = 0;

// gives the arch a chance to set things up the way it wants to
void ArchInterrupts::initialise()
{

/// set all the callbacks
//traps init
trap_init();
//event init

//stop event delivery

}

void ArchInterrupts::enableTimer()
{
  enable_ev_action( EV_TIMER );
}

void ArchInterrupts::disableTimer()
{
  disable_ev_action( EV_TIMER );
}

void ArchInterrupts::enableInterrupts()
{
  __sti();
}


bool ArchInterrupts::disableInterrupts()
{
  bool x;
  __save_and_cli(x);
  return(!x);
}

//tests if the InteruptFlag in EFLAGS is set
bool ArchInterrupts::testIFSet()
{
  uint32 ret_val;
  __save_flags(ret_val);
  return (!ret_val);
}


void ArchInterrupts::enableKBD()
{
  enable_ev_action( EV_CONSOLE );
}

void ArchInterrupts::disableKBD()
{
  disable_ev_action( VIRQ_CONSOLE );
}

void ArchInterrupts::enableBDS()
{
//FIXXXXXME
}

void ArchInterrupts::EndOfInterrupt(uint16 number){}

