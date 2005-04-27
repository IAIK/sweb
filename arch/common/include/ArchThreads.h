//----------------------------------------------------------------------
//  $Id: ArchThreads.h,v 1.3 2005/04/27 08:58:16 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchThreads.h,v $
//  Revision 1.2  2005/04/26 15:58:45  nomenquis
//  threads, scheduler, happy day
//
//  Revision 1.1  2005/04/24 16:58:03  nomenquis
//  ultra hack threading
//
//----------------------------------------------------------------------


#ifndef _ARCH_THREADS_H_
#define _ARCH_THREADS_H_

#include "types.h"

// this is where the thread info for task switching is stored
class ArchThreadInfo;
  
// this is where the thread itself will reside
class Thread; 
  

extern ArchThreadInfo *currentThreadInfo;
extern Thread *currentThread;

class ArchThreads
{
public:
  
  static void initialise();
  static void switchToThreadOnIret(Thread *thread);
  static void createThreadInfosKernelThread(ArchThreadInfo *&info, pointer start_function, pointer stack);
  static void yield();
  static uint32 testSetLock(uint32 &lock, uint32 new_value);

};








#endif
