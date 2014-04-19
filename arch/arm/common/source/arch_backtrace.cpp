//-------------------------------------------------------------------------------------*/
#include "kprintf.h"
#include "Thread.h"
#include "arch_backtrace.h"
#include "InterruptUtils.h"
#include "ArchThreads.h"
#include "mm/KernelMemoryManager.h" // for use of "kernel_end_address"
#include "ustl/umap.h"
#include "ArchCommon.h"
//-------------------------------------------------------------------------------------*/
struct StackFrame
{
  pointer pc;
  pointer lr;
  pointer sp;
  StackFrame *prev_fp;
};
//-------------------------------------------------------------------------------------*/
extern Thread* currentThread;
//-------------------------------------------------------------------------------------*/

int backtrace(pointer *call_stack, int size, Thread *thread, bool use_stored_registers)
{
  if (!call_stack ||
      (use_stored_registers && !thread) ||
      (!use_stored_registers && thread != currentThread) ||
      size <= 1)
    return 0;

  StackFrame *fp = 0;

  if (!use_stored_registers)
  {
    asm("mov %[v], fp" : [v]"=r" (fp));
  }
  else
    fp = (StackFrame*)thread->kernel_arch_thread_info_->r11; // r11 is the fp register in gcc ;)

    int i = 0;

  void *StackStart = (void*)((uint32)thread->stack_ + sizeof(thread->stack_)); // the stack "starts" at the high addresses...
  void *StackEnd = (void*)thread->stack_; // ... and "ends" at the lower ones.

  if (use_stored_registers)
    call_stack[i++] = thread->kernel_arch_thread_info_->pc;

  while (i < size && ADDRESS_BETWEEN(fp, StackEnd, StackStart))
  {
    call_stack[i++] = fp->pc;
    fp = fp->prev_fp;
  }

  return i;
}
