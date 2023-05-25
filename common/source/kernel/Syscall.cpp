#include "Syscall.h"

#include "File.h"
#include "ProcessRegistry.h"
#include "Scheduler.h"
#include "Terminal.h"
#include "VfsSyscall.h"
#include "debug_bochs.h"
#include "offsets.h"
#include "syscall-definitions.h"

#include "ArchMulticore.h"

#include "assert.h"

size_t Syscall::syscallException(size_t syscall_number,
                                 size_t arg1,
                                 size_t arg2,
                                 size_t arg3,
                                 size_t arg4,
                                 size_t arg5)
{
  size_t return_value = 0;

  // no debug print because these might occur very often
  if ((syscall_number != sc_sched_yield) && (syscall_number != sc_outline))
  {
      debug(SYSCALL,
            "CPU %zu: Syscall %zd called with arguments %zd(=%zx) %zd(=%zx) %zd(=%zx) "
            "%zd(=%zx) %zd(=%zx) by %s[%zu] (%p)\n",
            SMP::currentCpuId(), syscall_number, arg1, arg1, arg2, arg2, arg3, arg3, arg4,
            arg4, arg5, arg5, currentThread->getName(), currentThread->getTID(),
            currentThread);
  }

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
    case sc_lseek:
        return_value = lseek(arg1, arg2, arg3);
      break;
    case sc_outline:
      outline(arg1, arg2);
      break;
    case sc_trace:
      trace();
      break;
    case sc_getdents:
      return_value = getdents((int) arg1, (char*) arg2, arg3);
      break;
    case sc_getcpu:
      return_value = getcpu((size_t*)arg1, (size_t*)arg2, (void*)arg3);
      break;
    default:
      return_value = -1;
      kprintf("Syscall::syscallException: Unimplemented Syscall Number %zu\n", syscall_number);
  }
  return return_value;
}

ssize_t Syscall::getdents(int fd, char *buffer, size_t size)
{
    if(buffer && ((size_t)buffer >= USER_BREAK || (size_t)buffer + size > USER_BREAK))
        return -1;
    return VfsSyscall::getdents(fd, buffer, size);
}

[[noreturn]] void Syscall::exit(size_t exit_code)
{
  debug(SYSCALL, "Syscall::EXIT: called, exit_code: %zd\n", exit_code);
  currentThread->kill();

  assert(false && "Returned from currentThread->kill()");
}

size_t Syscall::write(size_t fd, pointer buffer, size_t size)
{
  //WARNING: this might fail if Kernel PageFaults are not handled
  if ((buffer >= USER_BREAK) || (buffer + size > USER_BREAK))
  {
    return -1U;
  }

  size_t num_written = 0;

  if (fd == fd_stdout) //stdout
  {
    debug(SYSCALL, "Syscall::write: %.*s\n", (int)size, (char*) buffer);
    kprintf("%.*s", (int)size, (char*) buffer);
    num_written = size;
  }
  else
  {
    num_written = VfsSyscall::write(fd, (char*) buffer, size);
  }
  return num_written;
}

size_t Syscall::read(size_t fd, pointer buffer, size_t count)
{
  if ((buffer >= USER_BREAK) || (buffer + count > USER_BREAK))
  {
    return -1U;
  }

  size_t num_read = 0;

  if (fd == fd_stdin)
  {
    //this doesn't! terminate a string with \0, gotta do that yourself
    num_read = currentThread->getTerminal()->readLine((char*) buffer, count);
    debug(SYSCALL, "Syscall::read: %.*s\n", (int)num_read, (char*) buffer);
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
  if (path >= USER_BREAK)
  {
    return -1U;
  }
  return VfsSyscall::open((char*) path, flags);
}

size_t Syscall::lseek(int fd, off_t offset, int whence)
{
    return VfsSyscall::lseek(fd, offset, whence);
}

void Syscall::outline(size_t port, pointer text)
{
  //WARNING: this might fail if Kernel PageFaults are not handled
  if (text >= USER_BREAK)
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
  // THIS METHOD IS FOR TESTING PURPOSES ONLY AND NOT MULTITHREADING SAFE!
  // AVOID USING IT AS SOON AS YOU HAVE AN ALTERNATIVE!

  // parameter check begin
  if (path >= USER_BREAK)
  {
    return -1U;
  }

  debug(SYSCALL, "Syscall::createprocess: path:%s sleep:%zd\n", (char*) path, sleep);
  ssize_t fd = VfsSyscall::open((const char*) path, O_RDONLY);
  if (fd == -1)
  {
    return -1U;
  }
  VfsSyscall::close(fd);
  // parameter check end

  size_t process_count = ProcessRegistry::instance()->processCount();
  int status = ProcessRegistry::instance()->createProcess((const char*) path);
  if (sleep)
  {
    while (ProcessRegistry::instance()->processCount() > process_count) // please note that this will fail ;)
    {
      Scheduler::instance()->yield();
    }
  }
  return status;
}

void Syscall::trace()
{
  currentThread->printBacktrace();
}


int Syscall::getcpu(size_t *cpu, size_t *node, [[maybe_unused]] void *tcache)
{
    if(((size_t)cpu >= USER_BREAK) ||
        ((size_t)cpu + sizeof(*cpu) >= USER_BREAK) ||
        ((size_t)node >= USER_BREAK) ||
        ((size_t)node + sizeof(*node) >= USER_BREAK))
    {
        return -1;
    }

    if(cpu != nullptr)
    {
        *cpu = SMP::currentCpuId();
    }

    if(node != nullptr)
    {
        *node = 0;
    }

    return 0;
}
