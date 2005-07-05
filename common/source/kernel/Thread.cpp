//----------------------------------------------------------------------
//  $Id: Thread.cpp,v 1.10 2005/07/05 17:29:48 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: Thread.cpp,v $
//  Revision 1.9  2005/05/31 17:29:16  nomenquis
//  userspace
//
//  Revision 1.8  2005/05/25 08:27:49  nomenquis
//  cr3 remapping finally really works now
//
//  Revision 1.7  2005/05/19 15:43:43  btittelbach
//  Ansätze für eine UserSpace Verwaltung
//
//  Revision 1.6  2005/05/16 20:37:51  nomenquis
//  added ArchMemory for page table manip
//
//  Revision 1.5  2005/05/10 08:53:50  nelles
//  stack trace experimenting
//
//  Revision 1.4  2005/04/27 08:58:16  nomenquis
//  locks work!
//  w00t !
//
//  Revision 1.3  2005/04/26 15:58:45  nomenquis
//  threads, scheduler, happy day
//
//  Revision 1.2  2005/04/23 22:20:30  btittelbach
//  just stuff
//
//  Revision 1.1  2005/04/23 21:27:12  nomenquis
//  commit for bernhard
//
//----------------------------------------------------------------------

#include "kernel/Thread.h"
#include "ArchCommon.h"
#include "console/kprintf.h"
#include "ArchThreads.h"

static void ThreadStartHack()
{
  currentThread->Run();
  kprintfd("ThreadStartHack: Panic, thread returned\n");
  for(;;);
}

Thread::Thread()
{
  kprintfd("Thread::Thread: Thread ctor, this is %x, stack is %x, sizeof stack is %x", this,stack_, sizeof(stack_));
  ArchCommon::bzero((pointer)stack_,sizeof(stack_),1);

  //kprintfd("Thread::Thread: After bzero\n");
  ArchThreads::createThreadInfosKernelThread(kernel_arch_thread_info_,(pointer)&ThreadStartHack,getStackStartPointer());
  switch_to_userspace_ = 0;
}

pointer Thread::getStackStartPointer()
{
  pointer stack = (pointer)stack_;
  stack += sizeof(stack_) - sizeof(uint32);
  return stack;
}
