//----------------------------------------------------------------------
//  $Id: ArchThreads.cpp,v 1.2 2005/04/26 10:23:54 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchThreads.cpp,v $
//  Revision 1.1  2005/04/24 16:58:03  nomenquis
//  ultra hack threading
//
//----------------------------------------------------------------------

#include "ArchThreads.h"
#include "ArchCommon.h"

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

ArchThreadInfo *currentThreadInfo;
Thread *currentThread;

ArchThreadInfo *thread_one;
ArchThreadInfo *thread_two;

void ArchThreads::initialise()
{
  // this _REALLY_ sucks, we have to create a thread info for the main kernel thread
  // otherwise, on the very first irq we go kaboom.
  
//  currentThreadInfo = new ArchThreadInfo();
  currentThreadInfo = (ArchThreadInfo*) new uint8[1024];
  

}

#define DPL_KERNEL  0
#define DPL_USER    3

void archCreateThread(ArchThreadInfo* ainfo, pointer programCounter, pointer pageDirectory, pointer stack_pointer)
{
  ArchCommon::bzero((pointer)ainfo,sizeof(ArchThreadInfo));
  
  ainfo->cs      = KERNEL_CS;
  ainfo->ds      = KERNEL_DS;
  ainfo->es      = KERNEL_DS;
  ainfo->ss      = KERNEL_SS;
  ainfo->eflags  = 0x200;
  ainfo->eax     = 0;
  ainfo->ecx     = 0;
  ainfo->edx     = 0;
  ainfo->ebx     = 0;
  ainfo->esi     = 0;
  ainfo->edi     = 0;
  ainfo->dpl     = DPL_KERNEL;
  ainfo->esp     = stack_pointer;
  ainfo->ebp     = stack_pointer;
  ainfo->eip     = programCounter;
  ainfo->cr3     = pageDirectory;

 /* fpu (=fninit) */
  ainfo->fpu[0] = 0xFFFF037F;
  ainfo->fpu[1] = 0xFFFF0000;
  ainfo->fpu[2] = 0xFFFFFFFF;
  ainfo->fpu[3] = 0x00000000;
  ainfo->fpu[4] = 0x00000000;
  ainfo->fpu[5] = 0x00000000;
  ainfo->fpu[6] = 0xFFFF0000;
}

extern "C" uint32   kernel_page_directory_start;

void ArchThreads::initDemo(pointer fun1, pointer fun2)
{
  thread_one = (ArchThreadInfo*)new uint8[sizeof(ArchThreadInfo)];
  thread_two = (ArchThreadInfo*)new uint8[sizeof(ArchThreadInfo)];

  uint32 *stack_one = new uint32[1028];
  uint32 *stack_two = new uint32[1028];

  ArchCommon::bzero((pointer)stack_one,1028*sizeof(uint32));
  ArchCommon::bzero((pointer)stack_two,1028*sizeof(uint32));
  
  
  archCreateThread(thread_one,fun1,((pointer)&kernel_page_directory_start)-2*1024*1024*1024+1024*1024,(pointer)&stack_one[1027]);
  archCreateThread(thread_two,fun2,((pointer)&kernel_page_directory_start)-2*1024*1024*1024+1024*1024,(pointer)&stack_two[1027]);
  
}

void ArchThreads::switchThreads()
{
  if (currentThreadInfo == thread_one)
    currentThreadInfo = thread_two;
  else
    currentThreadInfo = thread_one;
}
