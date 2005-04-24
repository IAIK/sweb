//----------------------------------------------------------------------
//  $Id: ArchInterrupts.cpp,v 1.5 2005/04/24 20:39:31 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchInterrupts.cpp,v $
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
 // disableInterrupts();
  initialise8259s();
  SegmentUtils::initialise();
  InterruptUtils::initialise();
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
  for (;;);

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
