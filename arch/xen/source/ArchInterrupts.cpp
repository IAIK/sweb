//----------------------------------------------------------------------
//  $Id: ArchInterrupts.cpp,v 1.2 2005/09/21 02:18:58 rotho Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchInterrupts.cpp,v $
//  Revision 1.1  2005/08/01 08:18:59  nightcreature
//  initial release, partly dummy implementation, needs changes
//
//
//----------------------------------------------------------------------

#include "ArchInterrupts.h"
#include "8259.h"
#include "structures.h"
#include "ports.h"
#include "InterruptUtils.h"
#include "SegmentUtils.h"

static uint32 interrupts_on = 0;
static uint32 timer_on = 0;

// gives the arch a chance to set things up the way it wants to
void ArchInterrupts::initialise()
{
//   uint16 i; // disableInterrupts();
//   initialise8259s();
//   SegmentUtils::initialise();
//   InterruptUtils::initialise();
//   for (i=0;i<16;++i)
//      disableIRQ(i);

}

void ArchInterrupts::enableTimer()
{
//   enableIRQ(0);
}

void ArchInterrupts::disableTimer()
{
//   disableIRQ(0);
}

void ArchInterrupts::enableInterrupts()
{
//      __asm__ __volatile__("sti"
//    :
//    :
//    );
}

bool ArchInterrupts::disableInterrupts()
{
//    uint32 ret_val;

//  __asm__ __volatile__("pushfl\n"
//                       "popl %0\n"
//                       "cli"
//  : "=a"(ret_val)
//  :);
 
// // return ret_val;
  return(false);
}

//tests if the InteruptFlag in EFLAGS is set
bool ArchInterrupts::testIFSet()
{
//   uint32 ret_val;
  
//   __asm__ __volatile__(
//   "pushfl\n"
//   "popl %0\n"
//   : "=a"(ret_val)
//   :);
  
//   return (ret_val & (1 << 9));  //testing IF Flag
  
  return(false);
}


void ArchInterrupts::enableKBD(){}
void ArchInterrupts::enableBDS(){}
void ArchInterrupts::disableKBD(){}
void ArchInterrupts::EndOfInterrupt(uint16 number){}

