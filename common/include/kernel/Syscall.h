/**
 * @file Syscall.h
 */

#ifndef _SYSCALL_H_
#define _SYSCALL_H_


#include <types.h>
#include "Thread.h"
#include "Scheduler.h"
#include "console/kprintf.h"


/**
 * @class Syscall
 *
 * This class is merely a container for the syscallHandler and its methods.
 * There should reasonably not be an instance of Syscall, instead the method
 * syscallException is static and should be called directly with Syscall::syscallException(...)
 * in turn calling its static methods within the class.
 *
 * @invariant syscallException must stay this way because it is called from InterruptUtils this way
 */

class Syscall
{
  public:
/**
 * syscallException takes the 6 max arguments transmitted from userspace
 * handles the Syscall appropiatly and returns a single value back to the
 * architecture specific interrupt handler
 *
 * @pre IF==1
 * @return value that gets returned to userspace (i.e. in register eax on x86)
 * @param syscall_number the first argument from userspace is the type of syscall
 * as defined in syscall-definitions.h
 */
  static uint32 syscallException(uint32 syscall_number, uint32 arg1, uint32 arg2, uint32 arg3, uint32 arg4, uint32 arg5);

/**
 * exit is a basic example of a method handling the exit syscall
 *
 * @pre IF==1
 * @param exit_code is the exit code transmitted from userspace, conceivably it
 *        could be stored or forwared to a waiting parent
 */
  static void exit(uint32 exit_code);

/**
 * write is a basic example of a method handling the write syscall
 *
 * @pre IF==1
 * @pre pointer < 2gb
 * @param fd File-Descriptor as described in syscall-definitions:
 *        fd_stdin,fd_stdout,fd_stderr or fd>2 is anything else a process has opened
 * @param buffer is a pointer to a userspace buffer
 * @param size is the size of the buffer
 */
  static uint32 write(uint32 fd, pointer buffer, uint32 size);

/**
 * read is a basic example of a method handling the read syscall
 *
 * @pre IF==1
 * @pre pointer < 2gb
 * @param fd File-Descriptor as described in syscall-definitions:
 *        fd_stdin,fd_stdout,fd_stderr or fd>2 is anything else a process has opened
 * @param buffer is a pointer to a userspace buffer
 * @param count is the maximum number of bytes to read
 */
  static uint32 read(uint32 fd, pointer buffer, uint32 count);

  //static uint32 clone();
  //static uint32 brk(..);
  //static void waitpid();
  //static uint32 open(...);
  //static void close(...);
  //etc...

  private:
  //helper functions
};

#endif
