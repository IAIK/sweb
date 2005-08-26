//----------------------------------------------------------------------
//   $Id: Syscall.h,v 1.3 2005/08/26 13:58:24 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: Syscall.h,v $
//  Revision 1.2  2005/08/07 16:47:25  btittelbach
//  More nice synchronisation Experiments..
//  RaceCondition/kprintf_nosleep related ?/infinite memory write loop Error still not found
//  kprintfd doesn't use a buffer anymore, as output_bochs blocks anyhow, should propably use some arch-specific interface instead
//
//  Revision 1.1  2005/08/03 09:54:43  btittelbach
//  Syscall Files, unfinished as of yet
//


#ifndef _SYSCALL_H_
#define _SYSCALL_H_


#include <types.h>
#include "Thread.h"
#include "Scheduler.h"
#include "../console/kprintf.h"

class Syscall
{
  public:
  static uint32 syscallException(uint32 syscall_number, uint32 arg1, uint32 arg2, uint32 arg3, uint32 arg4, uint32 arg5);
  
  static void execve();
  static uint32 clone();
  static void exit(uint32 exit_code);
  static uint32 write(uint32 fd, pointer buffer, uint32 size);
  static uint32 read(uint32 fd, pointer buffer, uint32 count);  
  //static waitpid();
  //static open();
  //static close();
  //etc...
  
  private:
  //helper functions
};

#endif
