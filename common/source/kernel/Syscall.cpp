//----------------------------------------------------------------------
//   $Id: Syscall.cpp,v 1.1 2005/08/03 09:54:43 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: Syscall.cpp,v $

#include "Syscall.h"
#include "syscall-definitions.h"
#include "assert.h"
#include "../console/kprintf.h"
#include "ArchCommon.h"

uint32 Syscall::syscallException(uint32 syscall_number, uint32 arg0, uint32 arg1, uint32 arg2)
{
  uint32 return_value=0;
  switch (syscall_number)
  {
    case sc_exit:
      exit(arg0);
      break;
    case sc_clone:
      return_value = clone();
      break;
    case sc_write:
      return_value =  write(arg0,arg1,arg2);
      break;
    case sc_read:
      return_value =  read(arg0,arg1,arg2);
      break;
    default:
      kprintf("Syscall::syscall_exception: Unimplemented Syscall Number %d\n",syscall_number);
  }
  return return_value;
}

void Syscall::exit(uint32 exit_code) 
{
  currentThread->kill();
}

uint32 Syscall::write(uint32 fd, pointer buffer, uint32 size)
{
  assert(buffer < 2U*1024U*1024U*1024U);
  if (fd == fd_stdout) //stdout
  {
    char *kernel_buffer = new char[size+1];
    kernel_buffer[size]=0;
    //don't really need to copy, but do it anyway
    ArchCommon::memcpy((pointer) kernel_buffer, buffer, size);
    kprintfd("Syscall::write: %s\n",kernel_buffer);
    kprintf("%s",kernel_buffer);
    delete kernel_buffer;
  }
  return size;
}

uint32 Syscall::read(uint32 fd, pointer buffer, uint32 count)
{
  uint32 num_read = 0;
  if (fd == fd_stdin) //stdout
  {
    
    
    
  }
  return num_read;
}

uint32 Syscall::clone() 
{
  uint32 child_pid = 999; //no pids yet
  
  //clone process
 
  return child_pid; //we are parent
}
