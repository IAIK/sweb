/**
 * @file Syscall.cpp
 */

#include "Syscall.h"
#include "syscall-definitions.h"
#include "assert.h"
#include "console/kprintf.h"
#include "ArchCommon.h"
#include "ArchInterrupts.h"
#include "console/Terminal.h"
#include "console/debug.h"

uint32 Syscall::syscallException(uint32 syscall_number, uint32 arg1, uint32 arg2, uint32 arg3, uint32 arg4, uint32 arg5)
{
  uint32 return_value=0;

  debug(SYSCALL,"Syscall %d called with arguments %d(=%x) %d(=%x) %d(=%x) %d(=%x) %d(=%x)\n",syscall_number, arg1, arg1, arg2, arg2, arg3, arg3, arg4, arg4, arg5, arg5);

  switch (syscall_number)
  {
    case sc_exit:
      exit(arg1);
      break;
    case sc_write:
      return_value =  write(arg1,arg2,arg3);
      break;
    case sc_read:
      return_value =  read(arg1,arg2,arg3);
      break;
    default:
      kprintf("Syscall::syscall_exception: Unimplemented Syscall Number %d\n",syscall_number);
  }
  return return_value;
}

void Syscall::exit(uint32 exit_code)
{
  debug(SYSCALL, "Syscall::EXIT: called, exit_code: %d\n",exit_code);
  currentThread->kill();
}

uint32 Syscall::write(uint32 fd, pointer buffer, uint32 size)
{
  //WARNING: this might fail if Kernel PageFaults are not handled
  assert(buffer < 2U*1024U*1024U*1024U);
  if (fd == fd_stdout) //stdout
  {
    debug(SYSCALL,"Syscall::write: %B\n",(char*) buffer,size);
    kprint_buffer((char*)buffer,size);
  }
  return size;
}

uint32 Syscall::read(uint32 fd, pointer buffer, uint32 count)
{
  assert(buffer < 2U*1024U*1024U*1024U);
  uint32 num_read = 0;
  if (fd == fd_stdin)
  {
    //this doesn't! terminate a string with \0, gotta do that yourself
    num_read = currentThread->getTerminal()->readLine((char*) buffer, count);
    debug(SYSCALL,"Syscall::read: %B\n",(char*) buffer,num_read);
    if(isDebugEnabled(SYSCALL))
    {
      for (uint32 c=0; c<num_read; ++c)
        kprintfd("%c(%x) ",((char*)buffer)[c],((char*)buffer)[c]);
      kprintfd("\n");
    }

  }
  return num_read;
}
