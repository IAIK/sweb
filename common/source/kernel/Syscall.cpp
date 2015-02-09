#include "Syscall.h"
#include "syscall-definitions.h"
#include "assert.h"
#include "kprintf.h"
#include "ArchCommon.h"
#include "ArchInterrupts.h"
#include "Terminal.h"
#include "debug.h"
#include "debug_bochs.h"
#include "VfsSyscall.h"
#include "UserProcess.h"
#include "ProcessRegistry.h"
#include "File.h"

size_t Syscall::syscallException(size_t syscall_number, size_t arg1, size_t arg2, size_t arg3, size_t arg4, size_t arg5)
{
  size_t return_value = 0;

  if (syscall_number != sc_sched_yield || syscall_number == sc_outline) // no debug print because these might occur very often
    debug(SYSCALL, "Syscall %d called with arguments %d(=%x) %d(=%x) %d(=%x) %d(=%x) %d(=%x)\n", syscall_number, arg1,
          arg1, arg2, arg2, arg3, arg3, arg4, arg4, arg5, arg5);

  switch (syscall_number)
  {
    case sc_sched_yield:
      Scheduler::instance()->yield();
      break;
    case sc_createprocess:
      return_value = createprocess(arg1, arg2);
      break;
    case sc_exit:
      exit(arg1);
      break;
    case sc_write:
      return_value = write(arg1, arg2, arg3);
      break;
    case sc_read:
      return_value = read(arg1, arg2, arg3);
      break;
    case sc_open:
      return_value = open(arg1, arg2);
      break;
    case sc_close:
      return_value = close(arg1);
      break;
    case sc_outline:
      outline(arg1, arg2);
      break;
    case sc_trace:
      trace();
      break;
    case sc_pseudols:
      VfsSyscall::readdir((const char*) arg1);
      break;
    default:
      kprintf("Syscall::syscall_exception: Unimplemented Syscall Number %d\n", syscall_number);
  }
  return return_value;
}

void Syscall::exit(size_t exit_code)
{
  debug(SYSCALL, "Syscall::EXIT: called, exit_code: %d\n", exit_code);
  currentThread->kill();
}

size_t Syscall::write(size_t fd, pointer buffer, size_t size)
{
  //WARNING: this might fail if Kernel PageFaults are not handled
  if ((buffer >= 2U * 1024U * 1024U * 1024U) || (buffer + size > 2U * 1024U * 1024U * 1024U))
  {
    return -1U;
  }
  if (fd == fd_stdout) //stdout
  {
    debug(SYSCALL, "Syscall::write: %.*s\n", size, (char*) buffer);
    kprintf("%.*s", size, buffer);
  }
  else
  {
    VfsSyscall::write(fd, (char*) buffer, size);
  }
  return size;
}

size_t Syscall::read(size_t fd, pointer buffer, size_t count)
{
  if ((buffer >= 2U * 1024U * 1024U * 1024U) || (buffer + count > 2U * 1024U * 1024U * 1024U))
  {
    return -1U;
  }
  size_t num_read = 0;
  if (fd == fd_stdin)
  {
    //this doesn't! terminate a string with \0, gotta do that yourself
    num_read = currentThread->getTerminal()->readLine((char*) buffer, count);
    debug(SYSCALL, "Syscall::read: %.*s\n", num_read, (char*) buffer);
  }
  else
  {
    num_read = VfsSyscall::read(fd, (char*) buffer, count);
  }
  return num_read;
}

size_t Syscall::close(size_t fd)
{
  return VfsSyscall::close(fd);
}

size_t Syscall::open(size_t path, size_t flags)
{
  if (path >= 2U * 1024U * 1024U * 1024U)
  {
    return -1U;
  }
  return VfsSyscall::open((char*) path, flags);
}

void Syscall::outline(size_t port, pointer text)
{
  //WARNING: this might fail if Kernel PageFaults are not handled
  if (text >= 2U * 1024U * 1024U * 1024U)
  {
    return;
  }
  if (port == 0xe9) // debug port
  {
    writeLine2Bochs((const char*) text);
  }
}

size_t Syscall::createprocess(size_t path, size_t sleep)
{
  // THIS METHOD IS FOR TESTING PURPOSES ONLY!
  // AVOID USING IT AS SOON AS YOU HAVE AN ALTERNATIVE!

  // parameter check begin
  if (path >= 2U * 1024U * 1024U * 1024U)
  {
    return -1U;
  }
  debug(SYSCALL, "Syscall::createprocess: path:%s sleep:%d\n", (char*) path, sleep);
  ssize_t fd = VfsSyscall::open((const char*) path, O_RDONLY);
  if (fd == -1)
  {
    return -1U;
  }
  VfsSyscall::close(fd);
  // parameter check end

  size_t process_count = ProcessRegistry::instance()->processCount();
  ProcessRegistry::instance()->createProcess((const char*) path);
  if (sleep)
  {
    while (ProcessRegistry::instance()->processCount() > process_count) // please note that this will fail ;)
    {
      Scheduler::instance()->yield();
    }
  }
  return 0;
}

void Syscall::trace()
{
  currentThread->printUserBacktrace();
}

