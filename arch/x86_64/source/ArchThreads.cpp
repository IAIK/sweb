/**
 * @file ArchThreads.cpp
 *
 */

#include "ArchThreads.h"
#include "ArchCommon.h"
#include "ArchMemory.h"
#include "kprintf.h"
#include "paging-definitions.h"
#include "offsets.h"
#include "assert.h"
#include "Thread.h"

extern PageMapLevel4Entry kernel_page_map_level_4[];

void ArchThreads::initialise()
{
  currentThreadInfo = (ArchThreadInfo*) new uint8[sizeof(ArchThreadInfo)];

}
void ArchThreads::setAddressSpace(Thread *thread, ArchMemory& arch_memory)
{
  assert(arch_memory.page_map_level_4_);
  thread->kernel_arch_thread_info_->cr3 = arch_memory.page_map_level_4_ * PAGE_SIZE;
  if (thread->user_arch_thread_info_)
    thread->user_arch_thread_info_->cr3 = arch_memory.page_map_level_4_ * PAGE_SIZE;
}

uint32 ArchThreads::getPageDirPointerTable(Thread *thread)
{
  return thread->kernel_arch_thread_info_->cr3 / PAGE_SIZE;
}


void ArchThreads::createThreadInfosKernelThread(ArchThreadInfo *&info, pointer start_function, pointer stack)
{
  info = (ArchThreadInfo*)new uint8[sizeof(ArchThreadInfo)];
  //kprintfd("info = %x, start_function = %x, stack = %x\n", info, start_function, stack);
  ArchCommon::bzero((pointer)info,sizeof(ArchThreadInfo));
  pointer pml4 = (pointer)VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4);

  info->cs      = KERNEL_CS;
  info->ds      = KERNEL_DS;
  info->es      = KERNEL_DS;
  info->ss      = KERNEL_SS;
  info->rflags  = 0x200;
  info->dpl     = DPL_KERNEL;
  info->rsp     = stack;
  info->rbp     = stack;
  info->rip     = start_function;
  info->cr3     = pml4;
  assert(info->cr3);

 /* fpu (=fninit) */
  info->fpu[0] = 0xFFFF037F;
  info->fpu[1] = 0xFFFF0000;
  info->fpu[2] = 0xFFFFFFFF;
  info->fpu[3] = 0x00000000;
  info->fpu[4] = 0x00000000;
  info->fpu[5] = 0x00000000;
  info->fpu[6] = 0xFFFF0000;
}

void ArchThreads::createThreadInfosUserspaceThread(ArchThreadInfo *&info, pointer start_function, pointer user_stack, pointer kernel_stack)
{
  info = (ArchThreadInfo*)new uint8[sizeof(ArchThreadInfo)];
  ArchCommon::bzero((pointer)info,sizeof(ArchThreadInfo));
  pointer pml4 = (pointer)VIRTUAL_TO_PHYSICAL_BOOT(kernel_page_map_level_4);

  info->cs      = USER_CS;
  info->ds      = USER_DS;
  info->es      = USER_DS;
  info->ss      = USER_SS;
  info->ss0     = KERNEL_SS;
  info->rflags  = 0x200;
  info->dpl     = DPL_USER;
  info->rsp     = user_stack;
  info->rbp     = user_stack;
  info->rsp0    = kernel_stack;
  info->rip     = start_function;
  info->cr3     = pml4;
  assert(info->cr3);

 /* fpu (=fninit) */
  info->fpu[0] = 0xFFFF037F;
  info->fpu[1] = 0xFFFF0000;
  info->fpu[2] = 0xFFFFFFFF;
  info->fpu[3] = 0x00000000;
  info->fpu[4] = 0x00000000;
  info->fpu[5] = 0x00000000;
  info->fpu[6] = 0xFFFF0000;
  //kprintfd("ArchThreads::create: values done\n");

}

void ArchThreads::cleanupThreadInfos(ArchThreadInfo *&info)
{
  //avoid NULL-Pointer
  if (info)
    delete info;
}

void ArchThreads::yield()
{
  __asm__ __volatile__("int $65"
  :
  :
  );
}

extern "C" uint32 arch_TestAndSet(uint32 new_value, uint32 *lock);
uint32 ArchThreads::testSetLock(uint32 &lock, uint32 new_value)
{
  return arch_TestAndSet(new_value, &lock);
}

uint32 ArchThreads::atomic_add(uint32 &value, int32 increment)
{
  int32 ret=increment;
  __asm__ __volatile__(
  "lock; xadd %0, %1;"
  :"=a" (ret), "=m" (value)
  :"a" (ret)
  :);
  return ret;
}

int32 ArchThreads::atomic_add(int32 &value, int32 increment)
{
  return (int32) ArchThreads::atomic_add((uint32 &) value, increment);
}

void ArchThreads::printThreadRegisters(Thread *thread, uint32 userspace_registers)
{
  ArchThreadInfo *info = userspace_registers?thread->user_arch_thread_info_:thread->kernel_arch_thread_info_;
  if (!info)
  {
    kprintfd("%sThread: %10x, has no %s registers\n",userspace_registers?"Kernel":"  User",thread,userspace_registers ? "userspace" : "kernelspace");
    return;
  }
  kprintfd("%sThread: %10x, info: %10x -- rax: %10x  rbx: %10x  rcx: %10x  rdx: %10x -- rsp: %10x  rbp: %10x  rsp0 %10x -- rip: %10x  rflg: %10x  cr3: %x\n",
           userspace_registers?"  User":"Kernel",thread,info,info->rax,info->rbx,info->rcx,info->rdx,info->rsp,info->rbp,info->rsp0,info->rip,info->rflags,info->cr3);

}
