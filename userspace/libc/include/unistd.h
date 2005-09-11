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
 * CVS Log Info for $RCSfile: unistd.h,v $
 *
 * $Id: unistd.h,v 1.2 2005/09/11 12:35:49 aniederl Exp $
 * $Log: unistd.h,v $
 * Revision 1.1  2005/09/11 10:55:07  aniederl
 * initial import of unistd.h
 *
 */


#ifndef unistd_h___
#define unistd_h___

#include "stdarg.h"


/**
 * NULL pointer constant
 *
 */
#define NULL 0


/**
 * Set file offset to offset, defined for lseek() and fcntl()
 *
 */
#define SEEK_SET 1

/**
 * Set file offset to current + offset, defined for lseek() and fcntl()
 *
 */
#define SEEK_CUR 2

/**
 * Set file offset to EOF + offset, defined for lseek() and fcntl()
 *
 */
#define SEEK_END 3


/**
 * Lock a section for exclusive use, defined for lockf()
 *
 */
#define F_LOCK 1

/**
 * Unlock locked sections, defined for lockf()
 *
 */
#define F_ULOCK 2

/**
 * Test section for locks by other processes, defined for lockf()
 *
 */
#define F_TEST 3

/**
 * Test and lock a section for exclusive use, defined for lockf()
 *
 */
#define F_TLOCK 4


/**
 * File number of stdin
 *
 */
#define STDIN_FILENO 0

/**
 * File number of stdin
 *
 */
#define STDOUT_FILENO 1

/**
 * File number of stdin
 *
 */
#define STDERR_FILENO 2


/**
 * Used for sizes of objects
 *
 */
#ifndef SIZE_T_DEFINED
#define SIZE_T_DEFINED
typedef unsigned int size_t;
#endif // SIZE_T_DEFINED

/**
 * Used for a count of bytes or an error indication
 *
 */
#ifndef SSIZE_T_DEFINED
#define SSIZE_T_DEFINED
typedef unsigned int ssize_t;
#endif // SSIZE_T_DEFINED

/**
 * Used for file sizes
 *
 */
typedef long int off_t;

/**
 * Used for process IDs and process group IDs
 *
 */
typedef int pid_t;

/**
 * Time in microseconds
 *
 */
typedef unsigned int useconds_t;

/**
 * Signed integral type large enough to hold any pointer
 *
 */
typedef int intptr_t;


/**
 * Creates a child process.
 * The new process will be nearly identical to the callee except for its
 * PID and PPID.
 * Resource utilizations are set to zero, file locks are not inherited.
 *
 */
extern pid_t fork();

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
extern int execl(const char *path, const char *arg, ...);

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
extern int execlp(const char *file, const char *arg, ...);

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
//extern int execle(const char *path, const char *arg, ..., char *const envp[]);
extern int execle(const char *path, const char *arg, ...);

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
extern int execv(const char *path, char *const argv[]);

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
extern int execvp(const char *file, char *const argv[]);

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
extern int execve(const char *path, char *const argv[], char *const envp[]);

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
void _exit(int status);

/**
 * Creates a new hard link to an existing file.
 * If new_path exists it will not be overwritten.
 *
 * @param old_path Path to file for linking
 * @param new_path Path to the link to create
 * @return 0 on success, -1 otherwise and errno is set appropriately
 *
 */
extern int link(const char *old_path, const char *new_path);

/**
 * Deletes a name from the filesystem.
 * If that name was the last link to a file and no processes have the file
 * open the file is deleted.
 * If a process has the file opened when the last link to it is deleted, the
 * file will be deleted after the last file descriptor referring to it is
 * closed.
 *
 * @param path the pathname to delete from the filesystem
 * @return 0 on success, -1 otherwise and errno is set appropriately
 *
 */
extern int unlink(const char *path);

/**
 * Deletes the given directory.
 * The given directory must be empty.
 *
 * @param path the path to the directory to delete
 * @return 0 on success, -1 otherwise and errno is set appropriately
 *
 */
extern int rmdir(const char *path);

/**
 * Closes the given file descriptor, so that it no longer refers to any file
 * and may be reused.
 * Any locks held on the file it was associated with, and owned by the process,
 * are removed.
 * If file_descriptor is the last copy of a particular file descriptor the
 * the resources associated with it are freed; if the descriptor was the last
 * reference to a file which has been removed using unlink() the file is
 * deleted.
 *
 * @param file_descriptor the file descriptor which should be closed
 * @return 0 on success, -1 otherwise and errno is set appropriately
 *
 */
extern int close(int file_descriptor);

/**
 * Creates a copy of the given file descriptor using the lowest-numbered
 * unused descriptor.
 * The copy shares locks, file position pointers and flags with the original
 * file descriptor but not the close-on-exec flag.
 *
 * @param file_descriptor the descriptor to copy
 * @return the new descriptor or -1 if an error occured (errno is as usually\
 set appropriately)
 *
 */
extern int dup(int file_descriptor);

/**
 * Creates a copy of the given file descriptor using the given new descriptor
 * and closing it first if necessary.
 * The copy shares locks, file position pointers and flags with the original
 * file descriptor but not the close-on-exec flag.
 *
 * @param old_file_descriptor the descriptor to copy
 * @param new_file_descriptor the descriptor which should be the copy
 * @return the new descriptor or -1 if an error occured (errno is as usually\
 set appropriately)
 *
 */
extern int dup2(int old_file_descriptor, int new_file_descriptor);

/**
 * Creates a pipe.
 * A pair of file descriptors pointing to a pipe inode is created and placed
 * in an array pointed to by the given array parameter.
 * The first element in the array is for reading and the second for writing.
 *
 * @param file_descriptor_array An array into which the two file descriptors\
 for the new pipe will be written
 * @return 0 on success, -1 otherwise and errno is set appropriately
 *
 */
extern int pipe(int file_descriptor_array[2]);

/**
 * Applies, tests or removes a POSIX lock on an open file.
 * The operation given by the command argument will be carried out on the file
 * referenced by the given file descriptor, determining the section of the
 * file to operate on by the length argument.
 * If length is positive the section stretches over pos to pos + length - 1,
 * if it is negative it goes from pos - length to pos - 1 where pos is the
 * current file position.
 * If length is zero the section extends from the current position to infinity,
 * encompassing the present and future end-of-file positions.
 *
 * Valid operations are:
 *  - F_LOCK   Locks a section for exclusive use
 *  - F_ULOCK  Unlocks locked sections
 *  - F_TEST   Tests sections for locks by other processes
 *  - F_TLOCK  Tests and locks a section for exclusive use
 *
 * @param file_descriptor file descriptor referencing the file to operate on
 * @param command an integer value describing the lock operation
 * @param length length of the section to operate on, can be zero and negative
 * @return 0 on success, -1 otherwise and errno is set appropriately
 *
 */
extern int lockf(int file_descriptor, int command, off_t length);

/**
 * Repositions the read/write file offset to the given offset value according
 * to the directive whence.
 *
 * Possible values for whence:
 *  - SEEK_SET  Set offset to the given offset
 *  - SEEK_CUR  Set offset to current position + given offset bytes
 *  - SEEK_END  Set offset to the end-of-file position + given offset bytes
 *
 * If data is written after the end of the file, the data in the gap between
 * the original end-of-file and the new data is returned as zero on reads.
 *
 * @param file_descriptor file descriptor referencing the file for operation
 * @param offset the offset to set
 * @param whence the directive how the offset will be set
 * @return the resulting offset location as measured in bytes from the\
 beginning of the file, or (off_t)-1 if an error occurs (errno will be set in\
 this case)
 *
 */
extern off_t lseek(int file_descriptor, off_t offset, int whence);

/**
 * Reads from a file descriptor.
 * The provided buffer is filled with data from the given file. The count
 * variable specifies the number of bytes to read.
 *
 * If the given file is capable of seeking, the read will start at the file
 * position associated with the descriptor. This offset is incremented by
 * the number of bytes actually read.
 *
 * @param file_descriptor file descriptor referencing the file to read
 * @param buffer the buffer where the read data will be placed
 * @param count the number of bytes to read
 * @return the number of bytes read on success, 0 if count is zero or the \
 offset for reading is after the end-of-file, and -1 if an error occured
 *
 */
extern ssize_t read(int file_descriptor, void *buffer, size_t count);

/**
 * Writes to a file descriptor.
 * Up to count bytes from the provided buffer are written to the given file.
 *
 * If the given file is capable of seeking, the write will start at the file
 * position associated with the descriptor. This offset is incremented by
 * the number of bytes actually written.
 *
 * @param file_descriptor file descriptor referencing the file to write
 * @param buffer the buffer where the write data will be placed
 * @param count the number of bytes to write
 * @param offset the absolute offset where the write operation starts
 * @return the number of bytes written on success, 0 if count is zero or\
 nothing was written, and -1 if an error occured
 *
 */
extern ssize_t write(int file_descriptor, const void *buffer, size_t count);

/**
 * Sets the end of the data segment to the given value if it is reasonable,
 * the system has enough memory and the process doesn't exceed ist maximum
 * data size.
 *
 * @param data_segment_end the address where the end of the data segment\
 should be set.
 * @return 0 on success, -1 otherwise and errno is set to ENOMEM
 *
 */
extern int brk(void *data_segment_end);

/**
 * Increments the program's data space by the given value bytes.
 * Providing an increment of 0 can be used to find the current location of the
 * program break.
 *
 * @param increment the number of bytes for incrementing the data space
 * @return a pointer to the start of the new area on success, -1 otherwise\
 and errno is set to ENOMEM
 *
 */
extern void *sbrk(intptr_t increment);


#endif // unistd_h___


