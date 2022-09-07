#include "assert.h"
#include "kprintf.h"
#include "panic.h"
#include "debug_bochs.h"
#include "Thread.h"
#include "ArchInterrupts.h"
#include "Scheduler.h"
#include "SystemState.h"

extern "C" void halt();

[[noreturn]] void sweb_assert(const char *condition, uint32 line, const char* file, const char* function)
{
  ArchInterrupts::disableInterrupts();
  system_state = KPANIC;
  kprintfd("KERNEL PANIC: Assertion %s failed in File %s, Function %s on Line %d\n", condition, file, function, line);
  if (currentThread != 0)
    currentThread->printBacktrace(false);
  while(1);
}
