//-------------------------------------------------------------------------------------*/
#include "kprintf.h"
#include "Thread.h"
#include "backtrace.h"
#include "InterruptUtils.h"
#include "ArchThreads.h"
#include "mm/KernelMemoryManager.h" // for use of "kernel_end_address"
#include "ustl/umap.h"
#include "ArchCommon.h"

int backtrace(pointer *call_stack, int size, Thread *thread,
    bool use_stored_registers)
{
  return 0;
}

int backtrace_user(pointer *call_stack, int size, Thread *thread,
    bool use_stored_registers)
{
  return 0;
}
