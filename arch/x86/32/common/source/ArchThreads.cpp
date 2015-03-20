#include "ArchThreads.h"
#include "ArchMemory.h"
#include "kprintf.h"
#include "paging-definitions.h"
#include "offsets.h"
#include "Thread.h"
#include "kstring.h"



void ArchThreads::initialise()
{
  currentThreadInfo = (ArchThreadInfo*) new uint8[sizeof(ArchThreadInfo)];
}

void ArchThreads::setAddressSpace(Thread *thread, ArchMemory& arch_memory)
{
  thread->kernel_arch_thread_info_->cr3 = arch_memory.getValueForCR3();
  if (thread->user_arch_thread_info_)
    thread->user_arch_thread_info_->cr3 = arch_memory.getValueForCR3();
}

void ArchThreads::createBaseThreadInfo(ArchThreadInfo *&info, pointer start_function, pointer stack)
{
  info = (ArchThreadInfo*)new uint8[sizeof(ArchThreadInfo)];
  memset((void*)info, 0, sizeof(ArchThreadInfo));
  pointer root_of_kernel_paging_structure = VIRTUAL_TO_PHYSICAL_BOOT(((pointer)ArchMemory::getRootOfKernelPagingStructure()));

  info->esp     = stack;
  info->ebp     = stack;
  info->eflags  = 0x200;
  info->eip     = start_function;
  info->cr3     = root_of_kernel_paging_structure;

  /* fpu (=fninit) */
  info->fpu[0] = 0xFFFF037F;
  info->fpu[1] = 0xFFFF0000;
  info->fpu[2] = 0xFFFFFFFF;
  info->fpu[3] = 0x00000000;
  info->fpu[4] = 0x00000000;
  info->fpu[5] = 0x00000000;
  info->fpu[6] = 0xFFFF0000;
}

void ArchThreads::createThreadInfosKernelThread(ArchThreadInfo *&info, pointer start_function, pointer stack)
{
  createBaseThreadInfo(info,start_function,stack);

  info->cs      = KERNEL_CS;
  info->ds      = KERNEL_DS;
  info->es      = KERNEL_DS;
  info->ss      = KERNEL_SS;
  info->dpl     = DPL_KERNEL;
}

void ArchThreads::createThreadInfosUserspaceThread(ArchThreadInfo *&info, pointer start_function, pointer user_stack, pointer kernel_stack)
{
  createBaseThreadInfo(info,start_function,user_stack);

  info->cs      = USER_CS;
  info->ds      = USER_DS;
  info->es      = USER_DS;
  info->ss      = USER_SS;
  info->dpl     = DPL_USER;
  info->ss0     = KERNEL_SS;
  info->esp0    = kernel_stack;
}

void ArchThreads::changeInstructionPointer(ArchThreadInfo *info, pointer function)
{
  info->eip = function;
}

void ArchThreads::yield()
{
  asm("int $65");
}

uint32 ArchThreads::testSetLock(uint32 &lock, uint32 new_value)
{
  return __sync_lock_test_and_set(&lock,new_value);
}

uint32 ArchThreads::atomic_add(uint32 &value, int32 increment)
{
  return __sync_fetch_and_add(&value,increment);
}

int32 ArchThreads::atomic_add(int32 &value, int32 increment)
{
  return __sync_fetch_and_add(&value,increment);
}

uint64 ArchThreads::atomic_add(uint64 &value, int64 increment)
{
  return __sync_fetch_and_add(&value,increment);
}

int64 ArchThreads::atomic_add(int64 &value, int64 increment)
{
  return __sync_fetch_and_add(&value,increment);
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
    kprintfd("Error, this thread's archthreadinfo is 0 for use userspace regs: %d\n",userspace_registers);
    return;
  }
  if (verbose)
  {
    kprintfd("\t\t%sThread: %10x, info: %10x\n"\
             "\t\t\t eax: %10x  ebx: %10x  ecx: %10x  edx: %10x\n"\
             "\t\t\t esp: %10x  ebp: %10x  esp0 %10x  eip: %10x\n"\
             "\t\t\teflg: %10x  cr3: %10x\n",
             userspace_registers?"User-":"Kernel",thread,info,info->eax,info->ebx,info->ecx,info->edx,info->esp,info->ebp,info->esp0,info->eip,info->eflags,info->cr3);
  }
  else
  {
    kprintfd("\t%sThread %10x: info %10x eax %10x ebp %10x esp %10x esp0 %10x eip %10x cr3 %10x\n",
             userspace_registers?" User-":"Kernel",thread,info,info->eax,info->ebp,info->esp,info->esp0,info->eip,info->cr3);
  }
}
