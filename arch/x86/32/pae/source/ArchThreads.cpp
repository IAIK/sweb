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



void ArchThreads::initialise()
{
  currentThreadInfo = (ArchThreadInfo*) new uint8[sizeof(ArchThreadInfo)];
}

extern "C" uint32 kernel_page_directory_pointer_table;

void ArchThreads::setAddressSpace(Thread *thread, ArchMemory& arch_memory)
{
  void* pdpt = arch_memory.page_dir_pointer_table_;
  // last 5 bits must be zero!
  assert(((uint32)pdpt & 0x1F) == 0);
  uint32 paddr = ArchMemory::get_PAddr_Of_VAddr_In_KernelMapping((uint32)pdpt);
  kprintfd("vaddr %x vtpb %x paddr %x\n", pdpt, VIRTUAL_TO_PHYSICAL_BOOT(((pointer)pdpt)), paddr);
  thread->kernel_arch_thread_info_->cr3 = paddr;
  if (thread->user_arch_thread_info_)
    thread->user_arch_thread_info_->cr3 = paddr;
}

void ArchThreads::createThreadInfosKernelThread(ArchThreadInfo *&info, pointer start_function, pointer stack)
{
  info = (ArchThreadInfo*)new uint8[sizeof(ArchThreadInfo)];
  ArchCommon::bzero((pointer)info,sizeof(ArchThreadInfo));
  pointer pdpt = VIRTUAL_TO_PHYSICAL_BOOT(((pointer)&kernel_page_directory_pointer_table));

  info->cs      = KERNEL_CS;
  info->ds      = KERNEL_DS;
  info->es      = KERNEL_DS;
  info->ss      = KERNEL_SS;
  info->eflags  = 0x200;
  info->eax     = 0;
  info->ecx     = 0;
  info->edx     = 0;
  info->ebx     = 0;
  info->esi     = 0;
  info->edi     = 0;
  info->dpl     = DPL_KERNEL;
  info->esp     = stack;
  info->ebp     = stack;
  info->eip     = start_function;
  info->cr3     = pdpt;

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
  info->eip = function;
}


void ArchThreads::createThreadInfosUserspaceThread(ArchThreadInfo *&info, pointer start_function, pointer user_stack, pointer kernel_stack)
{
  info = (ArchThreadInfo*)new uint8[sizeof(ArchThreadInfo)];
  ArchCommon::bzero((pointer)info,sizeof(ArchThreadInfo));
  pointer pdpt = VIRTUAL_TO_PHYSICAL_BOOT(((pointer)&kernel_page_directory_pointer_table));

  info->cs      = USER_CS;
  info->ds      = USER_DS;
  info->es      = USER_DS;
  info->ss      = USER_SS;
  info->ss0     = KERNEL_SS;
  info->eflags  = 0x200;
  info->eax     = 0;
  info->ecx     = 0;
  info->edx     = 0;
  info->ebx     = 0;
  info->esi     = 0;
  info->edi     = 0;
  info->dpl     = DPL_USER;
  info->esp     = user_stack;
  info->ebp     = user_stack;
  info->esp0    = kernel_stack;
  info->eip     = start_function;
  info->cr3     = pdpt;

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
