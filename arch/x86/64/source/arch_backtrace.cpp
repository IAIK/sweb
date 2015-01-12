#include "kprintf.h"
#include "Thread.h"
#include "backtrace.h"
#include "InterruptUtils.h"
#include "ArchThreads.h"
#include "KernelMemoryManager.h" // for use of "kernel_end_address"
#include "umap.h"
#include "ArchCommon.h"

int backtrace(pointer *call_stack __attribute__((unused)), int size __attribute__((unused)),
              Thread *thread __attribute__((unused)), bool use_stored_registers __attribute__((unused)))
{
  return 0;
}

int backtrace_user(pointer *call_stack __attribute__((unused)), int size __attribute__((unused)),
                   Thread *thread __attribute__((unused)), bool use_stored_registers __attribute__((unused)))
{
  return 0;
}
