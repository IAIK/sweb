//----------------------------------------------------------------------
//  $Id: ArchInterrupts.cpp,v 1.7 2005/04/25 22:40:19 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchInterrupts.cpp,v $
//  Revision 1.6  2005/04/25 21:15:41  nomenquis
//  lotsa changes
//
//  Revision 1.5  2005/04/24 20:39:31  nomenquis
//  cleanups
//
//  Revision 1.4  2005/04/24 16:58:03  nomenquis
//  ultra hack threading
//
//  Revision 1.3  2005/04/24 10:06:08  nomenquis
//  commit to compile on different machine
//
//  Revision 1.2  2005/04/23 20:32:30  nomenquis
//  timer interrupt works
//
//  Revision 1.1  2005/04/23 20:08:26  nomenquis
//  updates
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
  //uint16 i; // disableInterrupts();
  initialise8259s();
  SegmentUtils::initialise();
  InterruptUtils::initialise();
//  for (i=0;i<16;++i)
//     disableIRQint(i);

}

void ArchInterrupts::enableTimer()
{
  enableIRQ(0);
}

void ArchInterrupts::disableTimer()
{
  disableIRQ(0);
}

void ArchInterrupts::enableInterrupts()
{
     __asm__ __volatile__("sti"
   :
   :
   );
}

void ArchInterrupts::disableInterrupts()
{
   uint32 ret_val;

 __asm__ __volatile__("pushfl\n"
                      "popl %0\n"
                      "cli"
 : "=a"(ret_val)
 :);
 
// return ret_val;

}
