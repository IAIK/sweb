//----------------------------------------------------------------------
//  $Id: ArchInterrupts.cpp,v 1.5 2006/01/20 07:20:04 nightcreature Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchInterrupts.cpp,v $
//  Revision 1.4  2005/09/28 15:57:31  rotho
//  some work-progress (but qhacks too...)
//
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

//#include "events.h"

static uint32 interrupts_on = 0;
static uint32 timer_on = 0;

// gives the arch a chance to set things up the way it wants to
void ArchInterrupts::initialise()
{

}

void ArchInterrupts::enableTimer()
{
}

void ArchInterrupts::disableTimer()
{
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
}

void ArchInterrupts::disableKBD()
{
}

void ArchInterrupts::enableBDS()
{
//FIXXXXXME
}

void ArchInterrupts::EndOfInterrupt(uint16 number){}

