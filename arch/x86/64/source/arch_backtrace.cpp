#include "kprintf.h"
#include "Thread.h"
#include "backtrace.h"
#include "InterruptUtils.h"
#include "ArchThreads.h"
#include "KernelMemoryManager.h" // for use of "kernel_end_address"
#include "Loader.h"
#include "umap.h"
#include "ArchCommon.h"

extern Thread* currentThread;

struct StackFrame
{
   StackFrame *previous_frame;
   void *return_address;
};

int backtrace(pointer *call_stack, int size, Thread *thread, bool use_stored_registers)
{
  if (!call_stack || (use_stored_registers && !thread) || (!use_stored_registers && thread != currentThread) ||
      size <= 1 || (thread && thread->switch_to_userspace_))
  {
    return 0;
  }

  StackFrame *CurrentFrame;
  int i;

  if (!use_stored_registers)
  {
    __asm__ __volatile__(" \
       movq %%rbp, %0\n"
        : "=g" (CurrentFrame)
    );
    i = 0;
  }
  else
  {
    CurrentFrame = (StackFrame*)thread->kernel_registers_->rbp;
    call_stack[0] = thread->kernel_registers_->rip;
    i = 1;
  }

  void *StackStart = (void*)((size_t)thread->kernel_stack_ + sizeof(thread->kernel_stack_)); // the stack "starts" at the high addresses...
  void *StackEnd = (void*)thread->kernel_stack_; // ... and "ends" at the lower ones.
  void *StartAddress = (void*)USER_BREAK;
  void *EndAddress = (void*)ArchCommon::getFreeKernelMemoryEnd();

  if(StackEnd > EndAddress || StackStart < StartAddress)
    return 0;

  while (i < size &&
      ADDRESS_BETWEEN(CurrentFrame, StackEnd, StackStart) &&
      ADDRESS_BETWEEN(CurrentFrame->return_address, StartAddress, EndAddress))
  {
    call_stack[i++] = (pointer)CurrentFrame->return_address;
    CurrentFrame = CurrentFrame->previous_frame;
  }
  return i;
}

int backtrace_user(pointer *call_stack, int size, Thread *thread, bool /*use_stored_registers*/)
{
  if (!call_stack || !size || !thread->user_registers_)
    return 0;

  if (thread->user_registers_->rbp % sizeof(pointer))
  {
    debug(BACKTRACE, "stack no aligned. this could cause serious problems\n");
    return 0;
  }
  void *rbp = (void*)thread->user_registers_->rbp;
  StackFrame *CurrentFrame = (StackFrame*)rbp;
  StackFrame *CurrentFrameI = (StackFrame*)thread->loader_->arch_memory_.checkAddressValid((pointer)rbp);

  // the userspace stack is allowed to be anywhere in userspace
  void *StackStart = (void*)(USER_BREAK - 1);
  void *StackEnd = (void*)thread->user_registers_->rsp;

  int i = 1;
  CurrentFrame = (StackFrame*)thread->user_registers_->rbp;
  call_stack[0] = thread->user_registers_->rip;

  void *StartAddress = (void*)0x1;
  void *EndAddress = (void*)USER_BREAK;

  while (i < size &&
      ADDRESS_BETWEEN(CurrentFrame, StackEnd, StackStart) &&
      ADDRESS_BETWEEN(&CurrentFrame->return_address, StackEnd, StackStart) &&
      ADDRESS_BETWEEN(StackEnd, StartAddress, EndAddress) &&
      ADDRESS_BETWEEN(StackStart, StartAddress, EndAddress) &&
      CurrentFrameI)
  {
    pointer return_address;
    if ((pointer)CurrentFrameI%PAGE_SIZE == 0)
    {
      auto return_ptr = (pointer *)thread->loader_->arch_memory_.checkAddressValid((pointer)&CurrentFrameI->return_address);
      return_address = return_ptr ? *return_ptr : 0;
    }
    else
    {
      return_address = (pointer)CurrentFrameI->return_address;
    }

    if (!ADDRESS_BETWEEN(CurrentFrameI->return_address, StartAddress, EndAddress))
      break;

    call_stack[i++] = return_address;

    CurrentFrame = CurrentFrameI->previous_frame;

    CurrentFrameI = (StackFrame*)thread->loader_->arch_memory_.checkAddressValid((pointer)CurrentFrameI->previous_frame);
  }

  return i;
}
