#include "kprintf.h"
#include "Thread.h"
#include "backtrace.h"
#include "InterruptUtils.h"
#include "ArchThreads.h"
#include "KernelMemoryManager.h" // for use of "kernel_end_address"
#include "umap.h"
#include "ArchCommon.h"

struct StackFrame
{
   StackFrame *previous_frame;
   void *return_address;
};

extern Thread* currentThread;

int backtrace(pointer *call_stack, int size, Thread *thread, bool use_stored_registers)
{
  if (!call_stack ||
      (use_stored_registers && !thread) ||
      (!use_stored_registers && thread != currentThread) ||
      size <= 1)
    return 0;

  void *ebp = 0;

  if (!use_stored_registers)
  {
    __asm__ __volatile__(" \
       movl %%ebp, %0\n"
        : "=g" (ebp)
    );
  }
  else
    ebp = (void*)thread->kernel_arch_thread_info_->ebp;

  int i = 0;
  StackFrame *CurrentFrame = (StackFrame*)ebp;
  void *StackStart = (void*)((uint32)thread->stack_ + sizeof(thread->stack_)); // the stack "starts" at the high addresses...
  void *StackEnd = (void*)thread->stack_; // ... and "ends" at the lower ones.

  if (use_stored_registers)
    call_stack[i++] = thread->kernel_arch_thread_info_->eip;

  void *StartAddress = (void*)0x80000000;
  void *EndAddress = (void*)ArchCommon::getFreeKernelMemoryEnd();

  while (i < size &&
      ADDRESS_BETWEEN(CurrentFrame, StackEnd, StackStart) &&
      ADDRESS_BETWEEN(CurrentFrame->return_address, StartAddress, EndAddress) &&
      ADDRESS_BETWEEN(StackEnd, StartAddress, EndAddress) &&
      ADDRESS_BETWEEN(StackStart, StartAddress, EndAddress))
  {
    call_stack[i++] = (pointer)CurrentFrame->return_address;
    CurrentFrame = CurrentFrame->previous_frame;
  }

  return i;
}

int backtrace_user(pointer *call_stack, int size, Thread *thread, bool /*use_stored_registers*/)
{
  if (!call_stack || !size || thread != currentThread || !thread->user_arch_thread_info_)
    return 0;

  void *ebp = (void*)thread->user_arch_thread_info_->ebp;
  StackFrame *CurrentFrame = (StackFrame*)ebp;

  // the userspace stack is allowed to be anywhere in userspace
  void *StackStart = (void*) (2U*1024U*1024U*1024U - sizeof ( pointer )); // the stack "starts" at the high addresses...
  void *StackEnd = 0x0; // ... and "ends" at the lower ones.

  void *StartAddress = (void*)0x00000000;
  void *EndAddress = (void*)0x80000000;

  int i = 0;
  while (i < size &&
      ADDRESS_BETWEEN(CurrentFrame, StackEnd, StackStart) &&
      ADDRESS_BETWEEN(CurrentFrame->return_address, StartAddress, EndAddress) &&
      ADDRESS_BETWEEN(StackEnd, StartAddress, EndAddress) &&
      ADDRESS_BETWEEN(StackStart, StartAddress, EndAddress))
  {
    call_stack[i++] = (pointer)CurrentFrame->return_address;
    CurrentFrame = CurrentFrame->previous_frame;
  }

  return i;
}
