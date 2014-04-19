/**
 * @file assert.cpp
 *
 */

#include "assert.h"
#include "kprintf.h"
#include "panic.h"
#include "debug_bochs.h"
#include "Thread.h"
#include "ArchInterrupts.h"

extern Thread* currentThread;

void pre_new_sweb_assert(uint32 condition, uint32 line, const char* file)
{
  char const *error_string = "KERNEL PANIC: Assertion Failed in File:  on Line:      ";
  char line_string[5];
  if (!condition)
  {
    ArchInterrupts::disableInterrupts();
    extern uint32 boot_completed;
    boot_completed = 0;
    if (currentThread != 0)
      currentThread->printBacktrace(false);
    for ( ; ;) ;
  }
}


void sweb_assert(const char *condition, uint32 line, const char* file)
{
  ArchInterrupts::disableInterrupts();
  extern uint32 boot_completed;
  boot_completed = 0;
  kprintfd("KERNEL PANIC: Assertion %s failed in File %s on Line %d\n",condition, file, line);
  kprintf("KERNEL PANIC: Assertion %s failed in File %s on Line %d\n",condition, file, line);
  kpanict((uint8*) "Halting System\n");
}
