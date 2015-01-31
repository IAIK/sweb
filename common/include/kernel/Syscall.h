/**
 * @file Syscall.h
 */

#ifndef _SYSCALL_H_
#define _SYSCALL_H_


#include <types.h>
#include "Thread.h"
#include "Scheduler.h"
#include "kprintf.h"


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
  static size_t syscallException(size_t syscall_number, size_t arg1, size_t arg2, size_t arg3, size_t arg4, size_t arg5);

/**
 * exit is a basic example of a method handling the exit syscall
 *
 * @pre IF==1
 * @param exit_code is the exit code transmitted from userspace, conceivably it
 *        could be stored or forwared to a waiting parent
 */
  static void exit(size_t exit_code);

/**
 * write a text to a hardware i/o port (for reasons of testing and debugging)
 * important: only hardware port 0xe9 is allowed. generally you should not
 * allow user programs to access hardware this way.
 *
 * @pre IF==1
 * @pre pointer < 2gb
 * @param port the port to write on
 * @param text the text which will be written
 */
  static void outline(size_t port, pointer text);

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
  static size_t write(size_t fd, pointer buffer, size_t size);

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
  static size_t read(size_t fd, pointer buffer, size_t count);

/**
 * close is a basic example of a method handling the close syscall
 *
 * @pre IF==1
 * @param fd File-Descriptor as described in syscall-definitions:
 *        fd_stdin,fd_stdout,fd_stderr or fd>2 is anything else a process has opened
 */
  static size_t close(size_t fd);

/**
 * open is a basic example of a method handling the open syscall
 *
 * @pre IF==1
 * @pre path < 2gb
 * @param flags file system flags
 */
  static size_t open(size_t path, size_t flags);

/**
 * creates a new process
 *
 * @pre IF==1
 * @pre pointer < 2gb
 * @param path the path to the binary to open
 * @param sleep until the new process terminated
 * @return -1 upon error, 0 otherwise
 */
  static size_t createprocess(size_t path, size_t sleep);

  //static size_t clone();
  //static size_t brk(..);
  //static void waitpid();
  //static size_t open(...);
  //static void close(...);
  //etc...

  static void trace();

  private:
  //helper functions
};

#endif
