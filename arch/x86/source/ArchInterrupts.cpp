//----------------------------------------------------------------------
//  $Id: ArchInterrupts.cpp,v 1.11 2005/07/27 13:43:47 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchInterrupts.cpp,v $
//  Revision 1.10  2005/05/31 17:29:16  nomenquis
//  userspace
//
//  Revision 1.9  2005/04/25 23:09:18  nomenquis
//  fubar 2
//
//  Revision 1.8  2005/04/25 22:41:58  nomenquis
//  foobar
//
//  Revision 1.7  2005/04/25 22:40:19  btittelbach
//  Anti Warnings v0.1
//
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
  uint16 i; // disableInterrupts();
  initialise8259s();
  SegmentUtils::initialise();
  InterruptUtils::initialise();
  for (i=0;i<16;++i)
     disableIRQ(i);

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

//tests if the InteruptFlag in EFLAGS is set
bool ArchInterrupts::testIFSet()
{
  uint32 ret_val;
  
  __asm__ __volatile__(
  "pushfl\n"
  "popl %0\n"
  : "=a"(ret_val)
  :);
  
  return (ret_val & (1 << 9));  //testing IF Flag
  
}
