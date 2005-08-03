//----------------------------------------------------------------------
//   $Id: Syscall.h,v 1.1 2005/08/03 09:54:43 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: Syscall.h,v $

#include <types.h>
#include "Thread.h"
#include "Scheduler.h"
#include "../console/kprintf.h"

class Syscall
{
  public:
  static uint32 syscallException(uint32 syscall_number, uint32 arg0, uint32 arg1, uint32 arg2);
  
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
