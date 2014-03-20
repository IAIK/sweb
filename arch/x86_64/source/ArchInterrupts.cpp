/**
 * @file ArchInterrupts.cpp
 *
 */

#include "ArchInterrupts.h"
#include "8259.h"
#include "structures.h"
#include "ports.h"
#include "InterruptUtils.h"
#include "ArchThreads.h"

void ArchInterrupts::initialise()
{
  uint16 i;
  disableInterrupts();
  initialise8259s();
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

void ArchInterrupts::enableKBD()
{
  enableIRQ(1);
  enableIRQ(9);
}

void ArchInterrupts::enableBDS()
{
  enableIRQ(2);
  enableIRQ(9);
  enableIRQ(11);
  enableIRQ(14);
  enableIRQ(15);
}

void ArchInterrupts::disableKBD()
{
  disableIRQ(1);
}

void ArchInterrupts::EndOfInterrupt(uint16 number) 
{
  sendEOI(number);
}

void ArchInterrupts::enableInterrupts()
{
     __asm__ __volatile__("sti"
   :
   :
   );
}

bool ArchInterrupts::disableInterrupts()
{
   uint64 ret_val;

 __asm__ __volatile__("pushfq\n"
                      "popq %0\n"
                      "cli"
 : "=a"(ret_val)
 :);

return (ret_val & (1 << 9));  //testing IF Flag

}

bool ArchInterrupts::testIFSet()
{
  uint64 ret_val;

  __asm__ __volatile__(
  "pushfq\n"
  "popq %0\n"
  : "=a"(ret_val)
  :);

  return (ret_val & (1 << 9));  //testing IF Flag
}

void ArchInterrupts::yieldIfIFSet()
{
  extern uint32 boot_completed;
  if (boot_completed && currentThread && testIFSet())
  {
    ArchThreads::yield();
  }
  else
  {
    __asm__ __volatile__("nop");
  }
}
