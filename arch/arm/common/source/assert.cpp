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

__attribute__((noreturn)) void pre_new_sweb_assert(const char* condition, uint32 line, const char* file)
{
  char const *error_string = "KERNEL PANIC: Assertion Failed in File:  on Line:      ";
  writeLine2Bochs(error_string);
  writeLine2Bochs(condition);
  writeChar2Bochs('\n');
  writeLine2Bochs(file);
  writeChar2Bochs('\n');
  size_t i = 1000;
  do
  {
    i /= 10;
    if (line > i)
      writeChar2Bochs('0' + ((line / i) % 10));
  } while (i > 0);
  writeChar2Bochs('\n');
  ArchInterrupts::disableInterrupts();
  extern uint32 boot_completed;
  boot_completed = 0;
  if (currentThread != 0)
    currentThread->printBacktrace(false);
  while(1);
}


__attribute__((noreturn)) void sweb_assert(const char *condition, uint32 line, const char* file)
{
  ArchInterrupts::disableInterrupts();
  extern uint32 boot_completed;
  boot_completed = 0;
  kprintfd("KERNEL PANIC: Assertion %s failed in File %s on Line %d\n",condition, file, line);
  kprintf("KERNEL PANIC: Assertion %s failed in File %s on Line %d\n",condition, file, line);
  kpanict((uint8*) "Halting System\n");
  while(1);
}
