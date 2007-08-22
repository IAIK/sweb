//----------------------------------------------------------------------
//  $Id: Thread.h,v 1.15 2005/09/16 15:47:41 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: Thread.h,v $
//  Revision 1.14  2005/09/15 18:47:06  btittelbach
//  FiFoDRBOSS should only be used in interruptHandler Kontext, for everything else use FiFo
//  IdleThread now uses hlt instead of yield.
//
//  Revision 1.13  2005/09/06 09:56:50  btittelbach
//  +Thread Names
//  +stdin Test Example
//
//  Revision 1.12  2005/07/21 19:08:41  btittelbach
//  Jö schön, Threads u. Userprozesse werden ordnungsgemäß beendet
//  Threads können schlafen, Mutex benutzt das jetzt auch
//  Jetzt muß nur der Mutex auch überall verwendet werden
//
//  Revision 1.11  2005/07/12 21:05:38  btittelbach
//  Lustiges Spielen mit UserProgramm Terminierung
//
//  Revision 1.10  2005/06/14 18:22:37  btittelbach
//  RaceCondition anfälliges LoadOnDemand implementiert,
//  sollte optimalerweise nicht im InterruptKontext laufen
//
//  Revision 1.9  2005/05/31 17:29:16  nomenquis
//  userspace
//
//  Revision 1.8  2005/05/25 08:27:49  nomenquis
//  cr3 remapping finally really works now
//
//  Revision 1.7  2005/05/19 15:43:42  btittelbach
//  Ans�tze f�r eine UserSpace Verwaltung
//
//  Revision 1.6  2005/05/16 20:37:51  nomenquis
//  added ArchMemory for page table manip
//
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
#include "fs/FileSystemInfo.h"

enum ThreadState {Running, Sleeping, ToBeDestroyed};

class Thread;
class ArchThreadInfo;
class Loader;
class Terminal;
  
class Thread
{
friend class Scheduler;
public:
  
  Thread();
  virtual ~Thread();
  void kill();
  // runs whatever the user wants it to run;


public:
  
  virtual void Run()=0;

  ArchThreadInfo *kernel_arch_thread_info_;
  ArchThreadInfo *user_arch_thread_info_;
  uint32 stack_[2048];
  
  uint32 switch_to_userspace_;
public:  
  pointer getStackStartPointer();
  
  Loader *loader_;

  ThreadState state_;

  const char *getName()
  {
    if (name_)
      return name_;
    else
      return "<UNNAMED THREAD>";
  }
  
  Terminal *getTerminal();  
  void setTerminal(Terminal *my_term);
  
  FileSystemInfo *getFSInfo();
  void setFSInfo(FileSystemInfo *fs_info);

private:
  
  Thread(Thread const &);
  Thread &operator=(Thread const&);

  uint64 num_jiffies_;
  uint32 pid_;

  Terminal *my_terminal_;
  FileSystemInfo *fs_info_;

protected:
  char *name_;
};









#endif
