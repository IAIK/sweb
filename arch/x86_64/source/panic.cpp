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

// pointers to the start and end of the kernel code
extern uint8* LS_Code;
extern uint8* LS_Data;

pointer KERNEL_CODE_START  = (pointer)&LS_Code;
pointer KERNEL_CODE_END    = (pointer)&LS_Data;


void kpanict ( uint8 * message )
{
  ArchInterrupts::disableInterrupts();
  extern uint32 boot_completed;
  boot_completed = 0;

  uint64* stack = (uint64*) currentThread->getStackStartPointer();

  kprintfd("KPANICT: %s \n", message );
  kprintf("KPANICT: %s \n", message );

  currentThread->printBacktrace(false);


  Scheduler::instance()->printThreadList();

  ArchInterrupts::disableInterrupts();
  ArchInterrupts::disableTimer();
  //disable other IRQ's ???

  while (1)
  {
    asm("hlt");
  }

  kprintf("MAJOR KERNEL PANIC!: Should never reach here\n");

}
