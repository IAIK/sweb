//----------------------------------------------------------------------
//  $Id: ArchThreads.cpp,v 1.6 2005/05/31 17:29:16 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchThreads.cpp,v $
//  Revision 1.5  2005/05/25 08:27:48  nomenquis
//  cr3 remapping finally really works now
//
//  Revision 1.4  2005/04/27 08:58:16  nomenquis
//  locks work!
//  w00t !
//
//  Revision 1.3  2005/04/26 15:58:45  nomenquis
//  threads, scheduler, happy day
//
//  Revision 1.2  2005/04/26 10:23:54  nomenquis
//  kernel at 2gig again, not 2gig + 1m since were not using 4m pages anymore
//
//  Revision 1.1  2005/04/24 16:58:03  nomenquis
//  ultra hack threading
//
//----------------------------------------------------------------------

#include "ArchThreads.h"
#include "ArchCommon.h"
#include "kprintf.h"
#include "paging-definitions.h"
#include "offsets.h"
#include "Thread.h"

typedef struct ArchThreadInfo
{
  uint32  eip;       // 0
  uint32  cs;        // 4
  uint32  eflags;    // 8
  uint32  eax;       // 12
  uint32  ecx;       // 16
  uint32  edx;       // 20
  uint32  ebx;       // 24
  uint32  esp;       // 28
  uint32  ebp;       // 32
  uint32  esi;       // 36
  uint32  edi;       // 40
  uint32  ds;        // 44
  uint32  es;        // 48
  uint32  fs;        // 52
  uint32  gs;        // 56
  uint32  ss;        // 60
  uint32  dpl;       // 64
  uint32  esp0;      // 68
  uint32  ss0;       // 72
  uint32  cr3;       // 76
  uint32  fpu[27];   // 80
};



void ArchThreads::initialise()
{
  // this _REALLY_ sucks, we have to create a thread info for the main kernel thread
  // otherwise, on the very first irq we go kaboom.
  
//  currentThreadInfo = new ArchThreadInfo();
  currentThreadInfo = (ArchThreadInfo*) new uint8[sizeof(ArchThreadInfo)];
  

}

extern "C" uint32 kernel_page_directory_start;

void ArchThreads::setPageDirectory(Thread *thread, uint32 page_dir_physical_page)
{
  thread->kernel_arch_thread_info_->cr3 = page_dir_physical_page * PAGE_SIZE;
  if (thread->user_arch_thread_info_->cr3)thread->user_arch_thread_info_->cr3 = page_dir_physical_page * PAGE_SIZE;
  kprintf("setting cr3 in info to %x\n",page_dir_physical_page * PAGE_SIZE);
}

void ArchThreads::createThreadInfosKernelThread(ArchThreadInfo *&info, pointer start_function, pointer stack)
{
  kprintf("ArchThreads::create enter %x\n",info);
  info = (ArchThreadInfo*)new uint8[sizeof(ArchThreadInfo)];
  kprintf("alloc done %x\n",info);
  ArchCommon::bzero((pointer)info,sizeof(ArchThreadInfo));
  pointer pageDirectory = VIRTUAL_TO_PHYSICAL_BOOT(((pointer)&kernel_page_directory_start));
  kprintf("bzero done\n");
  kprintf("CR3 is %x\n",pageDirectory);
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
  info->cr3     = pageDirectory;

 /* fpu (=fninit) */
  info->fpu[0] = 0xFFFF037F;
  info->fpu[1] = 0xFFFF0000;
  info->fpu[2] = 0xFFFFFFFF;
  info->fpu[3] = 0x00000000;
  info->fpu[4] = 0x00000000;
  info->fpu[5] = 0x00000000;
  info->fpu[6] = 0xFFFF0000;
  kprintf("values done\n");
}

void ArchThreads::createThreadInfosUserspaceThread(ArchThreadInfo *&info, pointer start_function, pointer user_stack, pointer kernel_stack)
{
  kprintf("ArchThreads::create user enter %x\n",info);
  info = (ArchThreadInfo*)new uint8[sizeof(ArchThreadInfo)];
  kprintf("alloc done %x\n",info);
  ArchCommon::bzero((pointer)info,sizeof(ArchThreadInfo));
  pointer pageDirectory = VIRTUAL_TO_PHYSICAL_BOOT(((pointer)&kernel_page_directory_start));
  kprintf("bzero done\n");
  kprintf("CR3 is %x\n",pageDirectory);

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
  #warning fixme, kernel stack ptr needed
  info->esp0    = kernel_stack;
  info->eip     = start_function;
  info->cr3     = pageDirectory;

 /* fpu (=fninit) */
  info->fpu[0] = 0xFFFF037F;
  info->fpu[1] = 0xFFFF0000;
  info->fpu[2] = 0xFFFFFFFF;
  info->fpu[3] = 0x00000000;
  info->fpu[4] = 0x00000000;
  info->fpu[5] = 0x00000000;
  info->fpu[6] = 0xFFFF0000;
  kprintf("values done\n"); 
  


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
