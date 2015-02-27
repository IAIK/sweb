/**
 * @file panic.cpp
 *
 */

#include <types.h>
#include <Thread.h>
#include <Scheduler.h>
#include <ArchInterrupts.h>
#include "kprintf.h"
#include "panic.h"
#include "debug_bochs.h"

void kpanict ( uint8 * message )
{
  system_state = KPANIC;
  ArchInterrupts::disableInterrupts();

  size_t* stack = (size_t*) currentThread->getStackStartPointer();

  kprintfd("%s \n", message );
  kprintf("%s \n", message );

  kprintfd( "KPANICT: stack is > %x ",  stack );
  //kprintf( "KPANICT: stack is > %x ",  stack );

  if (currentThread)
    currentThread->printBacktrace(false);

  Scheduler::instance()->printThreadList();

  ArchInterrupts::disableInterrupts();
  ArchInterrupts::disableTimer();
  //disable other IRQ's ???

  for(;;);

  kprintf("MAJOR KERNEL PANIC!: Should never reach here\n");

  currentThread->printBacktrace(false);

  Scheduler::instance()->printThreadList();

}
