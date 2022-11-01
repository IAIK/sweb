#include "assert.h"
#include "kprintf.h"
#include "panic.h"
#include "debug_bochs.h"
#include "Thread.h"
#include "ArchInterrupts.h"
#include "Scheduler.h"
#include "SMP.h"
#include "ArchMulticore.h"
#include "SystemState.h"
#include "ArchCpuLocalStorage.h"

__attribute__((noreturn)) void pre_new_sweb_assert(const char* condition, uint32 line, const char* file)
{
  system_state = KPANIC;
  char const *error_string = "KERNEL PANIC: Assertion Failed in File:  on Line:      ";
  char line_string[5];
  ArchInterrupts::disableInterrupts();
  writeChar2Bochs('\n');
  writeLine2Bochs(condition);
  writeChar2Bochs('\n');
  writeLine2Bochs(error_string);
  writeChar2Bochs('\n');
  writeLine2Bochs(file);
  writeChar2Bochs('\n');
  if (currentThread)
    currentThread->printBacktrace(false);
  uint8 * fb = (uint8*)0xC00B8000;
  uint32 s=0;
  uint32 i=0;
  for (s=0; s<40; ++s)
  {
    fb[i++] = error_string[s];
    fb[i++] = 0x9f;
  }

  while (file && *file)
  {
    fb[i++] = *file++;
    fb[i++] = 0x9f;
  }

  for (s=40; s<54; ++s)
  {
    fb[i++] = error_string[s];
    fb[i++] = 0x9f;
  }

  i-=4;
  for (s=0; s < (sizeof(line_string) - 1); ++s)
  {
    line_string[s]=' ';
  }
  line_string[s]='\0';
  while (line>0)
  {
    fb[i++] = (uint8) ( 0x30 + (line%10) );
    fb[i] = 0x9f;
    line_string[--s] = ( 0x30 + (line%10) );
    i-=3;
    line /= 10;
  }
  writeLine2Bochs(line_string);
  writeChar2Bochs('\n');
  while(true)
      ArchCommon::halt();
  unreachable();
}

eastl::atomic_flag assert_print_lock;
__cpu bool in_assert = false;
bool in_assert_pre_cls = false;

[[noreturn]] void sweb_assert(const char *condition, uint32 line, const char* file, const char* function)
{
  bool prev_intr = ArchInterrupts::disableInterrupts();
  system_state = KPANIC;
  debug_print_to_fb = false;

  Thread* calling_thread = CpuLocalStorage::ClsInitialized() ? currentThread : nullptr;
  bool* in_assert_p = CpuLocalStorage::ClsInitialized() ? &in_assert : &in_assert_pre_cls;

  if (*in_assert_p) {
      assert_print_lock.clear(eastl::memory_order_release);
      kprintfd("PANIC LOOP: How did we get here?\n");
      while(true)
          ArchCommon::halt();
      unreachable();
  }
  *in_assert_p = true;

  if (SMP::numRunningCpus() > 1)
  {
      ArchMulticore::stopOtherCpus();
  }

  while (assert_print_lock.test_and_set(eastl::memory_order_acquire));

  if (calling_thread)
  {
      debug(BACKTRACE, "CPU %zu backtrace:\n", SMP::currentCpuId());
      if (!prev_intr)
          debug(BACKTRACE, "CAUTION: Assertion occurred in interrupt handler that is potentially unrelated to normal thread execution\n");
      calling_thread->printBacktrace(false);
  }

  kprintfd("KERNEL PANIC: Assertion %s failed in File %s, Function %s on Line %d, CPU %zd\n", condition, file, function, line, SMP::currentCpuId());
  kprintf("KERNEL PANIC: Assertion %s failed in File %s, Function %s on Line %d, CPU %zd\n", condition, file, function, line, SMP::currentCpuId());

  assert_print_lock.clear(eastl::memory_order_release);
  while(true)
      ArchCommon::halt();
  unreachable();
}
