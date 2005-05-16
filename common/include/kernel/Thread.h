//----------------------------------------------------------------------
//  $Id: Thread.h,v 1.6 2005/05/16 20:37:51 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: Thread.h,v $
//  Revision 1.5  2005/05/02 19:58:40  nelles
//  made GetStackPointer in Thread public
//  added panic.cpp
//
//  Revision 1.4  2005/04/27 08:58:16  nomenquis
//  locks work!
//  w00t !
//
//  Revision 1.3  2005/04/26 15:58:45  nomenquis
//  threads, scheduler, happy day
//
//  Revision 1.2  2005/04/24 10:06:09  nomenquis
//  commit to compile on different machine
//
//  Revision 1.1  2005/04/23 21:27:12  nomenquis
//  commit for bernhard
//
//----------------------------------------------------------------------

#ifndef _THREAD_H_
#define _THREAD_H_

#include "types.h"

class Thread;
// thanx mona!
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

typedef struct ArchThread
{
  ArchThreadInfo *thread_info;
  Thread *thread;
  
};

class Thread
{
friend class Scheduler;
public:
  
  Thread();
  

  // runs whatever the user wants it to run;


protected:
  
  virtual void Run()=0;

  ArchThreadInfo *arch_thread_info_;
  uint32 stack_[2048];
  
public:  
  pointer getStackStartPointer();
  
private:
  
  Thread(Thread const &);
  Thread &operator=(Thread const&);

  uint64 num_jiffies_;
  uint32 pid_;
  pointer page_directory_;


};









#endif
