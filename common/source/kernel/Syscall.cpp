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
#include "fs/VfsSyscall.h"
#include "UserProcess.h"
#include "MountMinix.h"

extern VfsSyscall vfs_syscall;

uint32 Syscall::syscallException(uint32 syscall_number, uint32 arg1, uint32 arg2, uint32 arg3, uint32 arg4, uint32 arg5)
{
  uint32 return_value=0;

  if (syscall_number != sc_sched_yield || syscall_number == sc_outline) // no debug print because these might occur very often
    debug(SYSCALL,"Syscall %d called with arguments %d(=%x) %d(=%x) %d(=%x) %d(=%x) %d(=%x)\n",syscall_number, arg1, arg1, arg2, arg2, arg3, arg3, arg4, arg4, arg5, arg5);

  switch (syscall_number)
  {
    case sc_sched_yield:
      Scheduler::instance()->yield();
      break;
    case sc_createprocess:
      return_value = createprocess(arg1,arg2);
      break;
    case sc_exit:
      exit(arg1);
      break;
    case sc_write:
      return_value = write(arg1,arg2,arg3);
      break;
    case sc_read:
      return_value = read(arg1,arg2,arg3);
      break;
    case sc_outline:
      outline(arg1,arg2);
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
  if ((buffer >= 2U*1024U*1024U*1024U) || (buffer+size > 2U*1024U*1024U*1024U))
  {
    return -1U;
  }
  if (fd == fd_stdout) //stdout
  {
    debug(SYSCALL,"Syscall::write: %B\n",(char*) buffer,size);
    kprint_buffer((char*)buffer,size);
  }
  return size;
}

uint32 Syscall::read(uint32 fd, pointer buffer, uint32 count)
{
  if ((buffer >= 2U*1024U*1024U*1024U) || (buffer+count > 2U*1024U*1024U*1024U))
  {
    return -1U;
  }
  uint32 num_read = 0;
  if (fd == fd_stdin)
  {
    //this doesn't! terminate a string with \0, gotta do that yourself
    num_read = currentThread->getTerminal()->readLineRaw((char*) buffer, count);
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

void Syscall::outline(uint32 port, pointer text)
{
  //WARNING: this might fail if Kernel PageFaults are not handled
  if (text >= 2U*1024U*1024U*1024U)
  {
    return;
  }
  if (port == 0xe9) // debug port
  {
    oh_writeStringDebugNoSleep((const char*)text);
  }
}

uint32 Syscall::createprocess(uint32 path, uint32 sleep)
{
  debug(SYSCALL,"Syscall::createprocess: path:%d sleep:%d\n",path,sleep);
  if (path >= 2U*1024U*1024U*1024U)
  {
    return -1U;
  }
  debug(SYSCALL,"Syscall::createprocess: path:%s sleep:%d\n",(char*) path,sleep);
  uint32 fd = vfs_syscall.open((const char*) path, O_RDONLY);
  if (fd == -1U)
  {
    return -1U;
  }
  vfs_syscall.close(fd);
  uint32 len = strlen((const char*) path) + 1;
  char* copy = new char[len];
  memcpy(copy, (const char*) path, len);
  Thread* thread = MountMinixAndStartUserProgramsThread::instance()->createProcess(copy);
  if (sleep)
  {
    while(Scheduler::instance()->checkThreadExists(thread)) // please note that this might fail ;)
    {
      Scheduler::instance()->yield();
    }
  }
  return 0;
}

