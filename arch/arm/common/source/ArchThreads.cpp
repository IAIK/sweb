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
#include "Thread.h"
#include "Scheduler.h"
#include "SpinLock.h"

SpinLock global_atomic_add_lock("");

extern "C" uint32 kernel_page_directory_start;

void ArchThreads::initialise()
{
  new (&global_atomic_add_lock) SpinLock("global_atomic_add_lock");
  currentThreadInfo = (ArchThreadInfo*) new uint8[sizeof(ArchThreadInfo)];
  pointer pageDirectory = VIRTUAL_TO_PHYSICAL_BOOT(((pointer)&kernel_page_directory_start));
  currentThreadInfo->ttbr0 = pageDirectory;
}

void ArchThreads::setAddressSpace(Thread *thread, ArchMemory& arch_memory)
{
  assert(arch_memory.page_dir_page_ != 0);
  assert((arch_memory.page_dir_page_ & 0x3) == 0); // has to be aligned to 4 pages
  thread->kernel_arch_thread_info_->ttbr0 = LOAD_BASE + arch_memory.page_dir_page_ * PAGE_SIZE;
  if (thread->user_arch_thread_info_)
    thread->user_arch_thread_info_->ttbr0 = LOAD_BASE + arch_memory.page_dir_page_ * PAGE_SIZE;
}

uint32 ArchThreads::getPageDirectory(Thread *thread)
{
  return thread->kernel_arch_thread_info_->ttbr0 / PAGE_SIZE;
}


void ArchThreads::createThreadInfosKernelThread(ArchThreadInfo *&info, pointer start_function, pointer stack)
{
  info = (ArchThreadInfo*)new uint8[sizeof(ArchThreadInfo)];
  ArchCommon::bzero((pointer)info,sizeof(ArchThreadInfo));
  pointer pageDirectory = VIRTUAL_TO_PHYSICAL_BOOT(((pointer)&kernel_page_directory_start));
  assert((pageDirectory) != 0);
  assert(((pageDirectory) & 0x3FFF) == 0);
  assert(!(start_function & 0x3));
  info->pc = start_function;
  info->lr = start_function;
  info->cpsr = 0x6000001F;
  info->sp = stack & ~0xF;
  info->sp0 = stack & ~0xF;
  info->ttbr0 = pageDirectory;
  assert((pageDirectory) != 0);
  assert(((pageDirectory) & 0x3FFF) == 0);
}

void ArchThreads::changeInstructionPointer(ArchThreadInfo *info, pointer function)
{
  info->pc = function;
  info->lr = function;
}

void ArchThreads::createThreadInfosUserspaceThread(ArchThreadInfo *&info, pointer start_function, pointer user_stack, pointer kernel_stack)
{
  info = (ArchThreadInfo*)new uint8[sizeof(ArchThreadInfo)];
  ArchCommon::bzero((pointer)info,sizeof(ArchThreadInfo));
  pointer pageDirectory = VIRTUAL_TO_PHYSICAL_BOOT(((pointer)&kernel_page_directory_start));
  assert((pageDirectory) != 0);
  assert(((pageDirectory) & 0x3FFF) == 0);
  assert(!(start_function & 0x3));
  info->pc = start_function;
  info->lr = start_function;
  info->cpsr = 0x60000010;
  info->sp = user_stack & ~0xF;
  info->r11 = user_stack & ~0xF; // r11 is the fp
  info->sp0 = kernel_stack & ~0xF;
  info->ttbr0 = pageDirectory;
  assert((pageDirectory) != 0);
  assert(((pageDirectory) & 0x3FFF) == 0);
}

void ArchThreads::cleanupThreadInfos(ArchThreadInfo *&info)
{
  //avoid NULL-Pointer
  if (info)
    delete info;
}

void ArchThreads::yield()
{
  asm("swi #0xffff");
}

extern "C" uint32 arch_TestAndSet(uint32, uint32, uint32 new_value, uint32 *lock);
uint32 ArchThreads::testSetLock(uint32 &lock, uint32 new_value)
{
  uint32 result;
  asm("swp %[r], %[n], [%[l]]" : [r]"=r"(result) : [n]"r"(new_value), [l]"r"(&lock));
  return result;
}

extern "C" uint32 arch_atomic_add(uint32, uint32, uint32 increment, uint32 *value);
uint32 ArchThreads::atomic_add(uint32 &value, int32 increment)
{
  global_atomic_add_lock.acquire("before atomic_add");
  uint32 result = value;
  value += increment;
  global_atomic_add_lock.release("after atomic_add");
  return result;
}

int32 ArchThreads::atomic_add(int32 &value, int32 increment)
{
  return (int32) ArchThreads::atomic_add((uint32 &) value, increment);
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
             "\t\t\tttbr0: %10x  pc: %10x  sp: %10x  lr: %10x  cpsr: %10x\n"\
             "\t\t\tr0:%10x r1:%10x r2:%10x r3:%10x r4:%10x r5:%10x r6:%10x r7:%10x r8:%10x r9:%10x r10:%10x r11:%10x r12:%10x\n",
             userspace_registers?"  User":"Kernel",thread,info,info->ttbr0,info->pc,info->sp,info->lr,info->cpsr,info->r0,info->r1,info->r2,info->r3,info->r4,info->r5,info->r6,info->r7,info->r8,info->r9,info->r10,info->r11,info->r12);

  }
  else
  {
    kprintfd("%sThread: %10x, info: %10x -- ttbr0: %10x  pc: %10x  sp: %10x  lr: %10x  cpsr: %10x -- r0:%10x r1:%10x r2:%10x r3:%10x r4:%10x r5:%10x r6:%10x r7:%10x r8:%10x r9:%10x r10:%10x r11:%10x r12:%10x\n",
             userspace_registers?"  User":"Kernel",thread,info,info->ttbr0,info->pc,info->sp,info->lr,info->cpsr,info->r0,info->r1,info->r2,info->r3,info->r4,info->r5,info->r6,info->r7,info->r8,info->r9,info->r10,info->r11,info->r12);
  }
}
