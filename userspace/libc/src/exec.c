// Projectname: SWEB
// Simple operating system for educational purposes
//
// Copyright (C) 2005  Andreas Niederl
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.



/**
 * CVS Log Info for $RCSfile: exec.c,v $
 *
 * $Id: exec.c,v 1.4 2005/09/20 14:39:56 aniederl Exp $
 * $Log: exec.c,v $
 * Revision 1.3  2005/09/20 13:48:42  aniederl
 * changed syscalls for using __syscall asm function
 *
 * Revision 1.2  2005/09/16 05:00:07  aniederl
 * syscall macros don't work with explicit array parameters (like argv[]), so changed them to pointers
 *
 * Revision 1.1  2005/09/14 22:37:18  aniederl
 * stubs for exec functions
 *
 */



#include "unistd.h"
#include "../../../common/include/kernel/syscall-definitions.h"
#include "sys/syscall.h"
#include "stdarg.h"
#include "stdlib.h"


/**
 * Array of pointers to the environment strings, terminated by a null pointer
 *
 */
//extern char **environ;

//----------------------------------------------------------------------
/**
 * Creates a child process.
 * The new process will be nearly identical to the callee except for its
 * PID and PPID.
 * Resource utilizations are set to zero, file locks are not inherited.
 *
 */
pid_t fork()
{
  return __syscall(sc_fork, 0x00, 0x00, 0x00, 0x00, 0x00);
}

//----------------------------------------------------------------------
/**
 * Replaces the current process image with a new one.
 * The arguments starting with arg are the arguments for the new image starting
 * with the filename of the file to be executed (by convention).
 * The list of arguments must be terminated by a NULL pointer (which has to be
 * of type char *.
 * The environment variables for the new process image are taken from the
 * external environ variable in the current process.
 * If this function returns, an error has occured.
 *
 * Example: execl("/bin/sh", "/bin/sh", (char *) NULL);
 *
 * @param path the path to the file which has to be executed
 * @param arg argument list, terminated by a NULL pointer
 * @return -1 and errno is set to indicate the error
 *
 */
int execl(const char *path, const char *arg, ...)
{
  return 0;
}

//----------------------------------------------------------------------
/**
 * Replaces the current process image with a new one.
 * Searches for an executable file if the specified file name does not contain
 * a slash (/) character. The search path is the path specified by the PATH
 * variable in the environment. If it is not specified, the default path
 * ':/bin:/usr/bin" is used.
 * The arguments starting with arg are the arguments for the new image starting
 * with the filename of the file to be executed (by convention).
 * The list of arguments must be terminated by a NULL pointer (which has to be
 * of type char *.
 * The environment variables for the new process image are taken from the
 * external environ variable in the current process.
 * If this function returns, an error has occured.
 *
 * Example: execlp("sh", "sh", (char *) NULL);
 *
 * @param path the path to the file which has to be executed
 * @param arg argument list, terminated by a NULL pointer
 * @return -1 and errno is set to indicate the error
 *
 */
int execlp(const char *file, const char *arg, ...)
{

  return 0;
}

//----------------------------------------------------------------------
/**
 * Replaces the current process image with a new one.
 * The arguments starting with arg are the arguments for the new image starting
 * with the filename of the file to be executed (by convention).
 * The list of arguments must be terminated by a NULL pointer (which has to be
 * of type char *. It is followed by a pointer to an array containing the
 * environment variables for the new process image.
 * If this function returns, an error has occured.
 *
 * Example: execl("/bin/sh", "/bin/sh", (char *) NULL, environment);
 *
 * @param file the file which has to be executed
 * @param arg argument list, terminated by a NULL pointer
 * @param envp an array holding the environment variables
 * @return -1 and errno is set to indicate the error
 *
 */
//int execle(const char *path, const char *arg, ..., char *const envp[]);
int execle(const char *path, const char *arg, ...)
{

  return 0;
}

//----------------------------------------------------------------------
/**
 * Replaces the current process image with a new one.
 * The values provided with the argv array are the arguments for the new
 * image starting with the filename of the file to be executed (by convention).
 * This pointer array must be terminated by a NULL pointer (which has to be
 * of type char *.
 * If this function returns, an error has occured.
 *
 * @param path path to the file to execute
 * @param argv an array containing the arguments
 * @return 0 on success, -1 otherwise and errno is set appropriately
 *
 */
int execv(const char *path, char *const argv[])
{

  return 0;
}

//----------------------------------------------------------------------
/**
 * Replaces the current process image with a new one.
 * Searches for an executable file if the specified file name does not contain
 * a slash (/) character. The search path is the path specified by the PATH
 * variable in the environment. If it is not specified, the default path
 * ':/bin:/usr/bin" is used.
 * The values provided with the argv array are the arguments for the new
 * image starting with the filename of the file to be executed (by convention).
 * This pointer array must be terminated by a NULL pointer (which has to be
 * of type char *.
 * If this function returns, an error has occured.
 *
 * @param file the file to execute
 * @param argv an array containing the arguments
 * @return 0 on success, -1 otherwise and errno is set appropriately
 *
 */
int execvp(const char *file, char *const argv[])
{

  return 0;
}

//----------------------------------------------------------------------
/**
 * Replaces the current process image with a new one.
 * The values provided with the argv array are the arguments for the new
 * image starting with the filename of the file to be executed (by convention).
 * This pointer array must be terminated by a NULL pointer (which has to be
 * of type char *. It is followed by a pointer to an array containing the
 * environment variables for the new process image.
 * If this function returns, an error has occured.
 *
 * @param path path to the file to execute
 * @param argv an array containing the arguments
 * @param envp an array holding the environment variables
 * @return 0 on success, -1 otherwise and errno is set appropriately
 *
 */
int execve(const char *path, char *const *argv, char *const *envp)
{
  return __syscall(sc_execve, (long) path, (long) argv, (long) envp, 0x00,
                   0x00);
}


//----------------------------------------------------------------------
/**
 * Terminates the calling process. Any open file descriptors belonging to the
 * process are closed, any children of the process are inherited by process
 * 1, init, and the process's parent is sent a SIGCHLD signal.
 * The value status is returned to the parent process as the process's exit
 * status and can be collected using a wait call.
 * This function does NOT call any functions registered with atexit(), nor any
 * registered signal handlers.
 *
 * @param status exit status of the process
 *
 */
void _exit(int status)
{
  __syscall(sc_exit, status, 0x00, 0x00, 0x00, 0x00);
}

//----------------------------------------------------------------------
/**
 * Terminates the program normally, functions registered by atexit() are called
 * in reverse order of their registration, any open file descriptors belonging
 * to the process are closed, any children of the process are inherited by
 * process 1 (init) and the process's parent is sent a SIGCHLD signal.
 * The status value is returned to the parent as exit status and can be
 * collected using a wait call.
 * @param status the exit status of the calling process, 0 or EXIT_SUCCESS for
 * successfull termination, EXIT_FAILURE for unsuccessful termination
 *
 */
void exit(int status)
{
  __syscall(sc_exit, status, 0x00, 0x00, 0x00, 0x00);
}
