#include "panic.h"

#include "SystemState.h"
#include "debug_bochs.h"
#include "kprintf.h"
#include <Scheduler.h>
#include <Thread.h>

#include <ArchInterrupts.h>

#include <types.h>

void kpanict ( const char * message )
{
  system_state = KPANIC;
  ArchInterrupts::disableInterrupts();

  size_t* stack = (size_t*) currentThread->getKernelStackStartPointer();

  kprintfd("%s \n", message );
  kprintf("%s \n", message );

  kprintfd( "KPANICT: stack is > %p ",  stack );
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
