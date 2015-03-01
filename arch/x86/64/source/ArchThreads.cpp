#include "ArchThreads.h"
#include "ArchMemory.h"
#include "kprintf.h"
#include "paging-definitions.h"
#include "offsets.h"
#include "assert.h"
#include "Thread.h"
#include "kstring.h"

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
  memset((void*)info, 0, sizeof(ArchThreadInfo));
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

void ArchThreads::changeInstructionPointer(ArchThreadInfo *info, pointer function)
{
  info->rip = function;
}

void ArchThreads::createThreadInfosUserspaceThread(ArchThreadInfo *&info, pointer start_function, pointer user_stack, pointer kernel_stack)
{
  info = (ArchThreadInfo*)new uint8[sizeof(ArchThreadInfo)];
  memset((void*)info, 0, sizeof(ArchThreadInfo));
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

void ArchThreads::yield()
{
  __asm__ __volatile__("int $65"
  :
  :
  );
}

size_t ArchThreads::testSetLock(size_t &lock, size_t new_value)
{
  return __sync_lock_test_and_set(&lock,new_value);
}

uint64 ArchThreads::atomic_add(uint64 &value, int64 increment)
{
  int32 ret=increment;
  __asm__ __volatile__(
  "lock; xadd %0, %1;"
  :"=a" (ret), "=m" (value)
  :"a" (ret)
  :);
  return ret;
}

int64 ArchThreads::atomic_add(int64 &value, int64 increment)
{
  return (int64) ArchThreads::atomic_add((uint64 &) value, increment);
}

void ArchThreads::atomic_set(uint32& target, uint32 value)
{
  __atomic_store_n(&(target), value, __ATOMIC_SEQ_CST);
}

void ArchThreads::atomic_set(int32& target, int32 value)
{
  __atomic_store_n(&(target), value, __ATOMIC_SEQ_CST);
}

void ArchThreads::atomic_set(uint64& target, uint64 value)
{
  __atomic_store_n(&(target), value, __ATOMIC_SEQ_CST);
}

void ArchThreads::atomic_set(int64& target, int64 value)
{
  __atomic_store_n(&(target), value, __ATOMIC_SEQ_CST);
}

void ArchThreads::printThreadRegisters(Thread *thread, bool verbose)
{
  printThreadRegisters(thread,0,verbose);
  printThreadRegisters(thread,1,verbose);
}

void ArchThreads::printThreadRegisters(Thread *thread, uint32 userspace_registers, bool verbose)
{
  ArchThreadInfo *info = userspace_registers?thread->user_arch_thread_info_:thread->kernel_arch_thread_info_;
  if (!info)
  {
    kprintfd("%sThread: %18x, has no %s registers\n",userspace_registers?"Kernel":"  User",thread,userspace_registers ? "userspace" : "kernelspace");
    return;
  }
  if (verbose)
  {
    kprintfd("\t\t%sThread: %18x, info: %18x\n"\
             "\t\t\t rax: %18x  rbx: %18x  rcx: %18x  rdx: %18x\n"\
             "\t\t\t rsp: %18x  rbp: %18x  rsp0 %18x  rip: %18x\n"\
             "\t\t\trflg: %18x  cr3: %x\n",
             userspace_registers?"  User":"Kernel",thread,info,info->rax,info->rbx,info->rcx,info->rdx,info->rsp,info->rbp,info->rsp0,info->rip,info->rflags,info->cr3);
  }
  else
  {
    kprintfd("%sThread: %18x, info: %18x -- rax: %18x  rbx: %18x  rcx: %18x  rdx: %18x -- rsp: %18x  rbp: %18x  rsp0 %18x -- rip: %18x  rflg: %18x  cr3: %x\n",
             userspace_registers?"  User":"Kernel",thread,info,info->rax,info->rbx,info->rcx,info->rdx,info->rsp,info->rbp,info->rsp0,info->rip,info->rflags,info->cr3);
  }
}
