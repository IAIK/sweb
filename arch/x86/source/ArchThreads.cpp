//----------------------------------------------------------------------
//  $Id: ArchThreads.cpp,v 1.11 2005/08/26 13:58:24 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchThreads.cpp,v $
//  Revision 1.10  2005/07/26 17:45:25  nomenquis
//  foobar
//
//  Revision 1.9  2005/07/21 19:08:40  btittelbach
//  Jö schön, Threads u. Userprozesse werden ordnungsgemäß beendet
//  Threads können schlafen, Mutex benutzt das jetzt auch
//  Jetzt muß nur der Mutex auch überall verwendet werden
//
//  Revision 1.8  2005/07/05 17:29:48  btittelbach
//  new kprintf(d) Policy:
//  [Class::]Function: before start of debug message
//  Function can be abbreviated "ctor" if Constructor
//  use kprintfd where possible
//
//  Revision 1.7  2005/06/14 18:22:37  btittelbach
//  RaceCondition anfälliges LoadOnDemand implementiert,
//  sollte optimalerweise nicht im InterruptKontext laufen
//
//  Revision 1.6  2005/05/31 17:29:16  nomenquis
//  userspace
//
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
  if (thread->user_arch_thread_info_->cr3)
    thread->user_arch_thread_info_->cr3 = page_dir_physical_page * PAGE_SIZE;
  kprintfd_nosleep("ArchThreads::setPageDirectory: setting cr3 in info to %x\n",page_dir_physical_page * PAGE_SIZE);
}

uint32 ArchThreads::getPageDirectory(Thread *thread)
{
  return thread->kernel_arch_thread_info_->cr3 / PAGE_SIZE;
  //FIXXME: should be the same for now, have to return only one 
}


void ArchThreads::createThreadInfosKernelThread(ArchThreadInfo *&info, pointer start_function, pointer stack)
{
  kprintfd_nosleep("ArchThreads::createThreadInfosKernelThread: enter %x\n",info);
  info = (ArchThreadInfo*)new uint8[sizeof(ArchThreadInfo)];
  kprintfd_nosleep("ArchThreads::createThreadInfosKernelThread: alloc done %x\n",info);
  ArchCommon::bzero((pointer)info,sizeof(ArchThreadInfo));
  pointer pageDirectory = VIRTUAL_TO_PHYSICAL_BOOT(((pointer)&kernel_page_directory_start));
  kprintfd_nosleep("ArchThreads::createThreadInfosKernelThread: bzero done\n");
  kprintfd_nosleep("ArchThreads::createThreadInfosKernelThread: CR3 is %x\n",pageDirectory);
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
  kprintfd_nosleep("ArchThreads::createThreadInfosKernelThread: values done\n");
}

void ArchThreads::createThreadInfosUserspaceThread(ArchThreadInfo *&info, pointer start_function, pointer user_stack, pointer kernel_stack)
{
  kprintfd_nosleep("ArchThreads::create: user enter %x\n",info);
  info = (ArchThreadInfo*)new uint8[sizeof(ArchThreadInfo)];
  kprintfd_nosleep("ArchThreads::create:alloc done %x\n",info);
  ArchCommon::bzero((pointer)info,sizeof(ArchThreadInfo));
  pointer pageDirectory = VIRTUAL_TO_PHYSICAL_BOOT(((pointer)&kernel_page_directory_start));
  kprintfd_nosleep("ArchThreads::create: bzero done\n");
  kprintfd_nosleep("ArchThreads::create: CR3 is %x\n",pageDirectory);

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
  kprintfd_nosleep("ArchThreads::create: values done\n"); 
  
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


void ArchThreads::printThreadRegisters(Thread *thread, uint32 userspace_registers)
{
  ArchThreadInfo *info = userspace_registers?thread->user_arch_thread_info_:thread->kernel_arch_thread_info_;
  if (!info)
  {
    kprintfd_nosleep("Error, this thread's archthreadinfo is 0 for use userspace regs: %d\n",userspace_registers);
    return;
  }
  kprintfd("\n\n");
  kprintfd("Thread: %x, info %x %s\n",thread,info,userspace_registers?"UserSpace":"Kernel");
  kprintfd("eax: %x  ebx: %x  ecx: %x  edx: %x\n",info->eax,info->ebx,info->ecx,info->edx);
  kprintfd("esi: %x  edi: %x  esp: %x  ebp: %x\n",info->esi,info->edi,info->esp,info->ebp);
  kprintfd(" ds: %x   es: %x   fs: %x   gs: %x\n",info->ds,info->es,info->fs,info->gs);
  kprintfd(" ss: %x   cs: %x esp0: %x  ss0: %x\n",info->ss,info->cs,info->esp0,info->ss0);
  kprintfd("eip: %x eflg: %x  dpl: %x  cr3: %x\n",info->eip,info->eflags,info->dpl,info->cr3);
  kprintfd("\n\n");  
}

