//----------------------------------------------------------------------
//  $Id: ArchThreads.h,v 1.1 2005/04/24 16:58:03 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: $
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
  static void initDemo(pointer fun1, pointer fun2);
  static void switchThreads();

};








#endif
