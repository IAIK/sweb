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
 * CVS Log Info for $RCSfile: fcntl.h,v $
 *
 * $Id: fcntl.h,v 1.2 2005/09/13 17:31:04 aniederl Exp $
 * $Log: fcntl.h,v $
 * Revision 1.1  2005/09/11 13:15:46  aniederl
 * import of fcntl.h
 *
 */


#ifndef fcntl_h___
#define fcntl_h___

/**
 * Duplicate file descriptor
 *
 */
#define F_DUPFD 1

/**
 * Get file descriptor flags
 *
 */
#define F_GETFD 2

/**
 * Set file descriptor flags
 *
 */
#define F_SETFD 3

/**
 * Get file status flags and file access mode
 *
 */
#define F_GETFL 4

/**
 * Set file status flags
 *
 */
#define F_SETFL 5

/**
 * Get record locking information
 *
 */
#define F_GETLK 6

/**
 * Set record locking information
 *
 */
#define F_SETLK 7

/**
 * Set record locking information; wait if blocked
 *
 */
#define F_SETLKW 8

/**
 * Close the file descriptor upon execution of an exec family function
 *
 */
#define FD_CLOEXEC 11

/**
 * Shared or read lock
 *
 */
#define F_RDLCK 12

/**
 * Unlock
 *
 */
#define F_UNLCK 13

/**
 * Exclusive or write lock
 *
 */
#define F_WRLCK 14

/**
 * Create file if it does not exist
 *
 */
#define O_CREAT 1

/**
 * Truncate flag
 *
 */
#define O_TRUNC 4

/**
 * Set append mode
 *
 */
#define O_APPEND 5

/**
 * Non-blocking mode
 *
 */
#define O_NONBLOCK 7

/**
 * Synchronized read I/O operations
 *
 */
#define O_RSYNC 8

/**
 * Mask for file access modes
 *
 */
#define O_ACCMODE 10

/**
 * Open for reading only
 *
 */
#define O_RDONLY 11

/**
 * Open for reading and writing
 *
 */
#define O_RDWR 12

/**
 * Open for writing only
 *
 */
#define O_WRONLY 13

/**
 * Structure for describing a file lock
 *
 */
struct flock
{

  /**
   * Type of lock
   * F_RDLCK, F_WRLCK or F_UNLCK
   *
   */
  short l_type;

  /**
   * Flag for starting offset
   *
   */
  short l_whence;

  /**
   * Relative offset in bytes
   *
   */
  off_t l_start;

  /**
   * Size, if 0 then until EOF
   *
   */
  off_t l_len;

  /**
   * Process ID of the process holding the lock.
   *
   */
  pid_t l_pid;

};

/**
 * Used for file attributes
 *
 */
typedef unsigned int mode_t;

/**
 * Equivalent to open() with flags equal to O_CREAT | O_WRONLY | O_TRUNC.
 *
 * @param path A pathname pointing to the file to open
 * @param mode
 * @return A valid file descriptor or -1 if an error occured
 *
 */
extern int creat(const char *path, mode_t mode);

/**
 * Converts a pathname into a file descriptor which can be used in subsequent
 * read and write operations.
 * If successfull, the lowest file descriptor not currently open for the
 * process will be returned.
 *
 * Possible values for the flags parameter are:
 *  - O_RDONLY  Open for reading only
 *  - O_WRONLY  Open for writing only
 *  - O_RDWR    Open for reading and writing
 *
 * which can be bitwise combined (or'd) with:
 *  - O_APPEND  Open in append mode
 *  - O_CREAT   Create file if it does not exist
 *  - O_TRUNC   Truncate file to 0 length if it already exists and is a
 *              regular file
 *
 * @param path A pathname pointing to the file to open
 * @param flags Flags to specify how the file is opened
 * @return A valid file descriptor or -1 if an error occured
 *
 */
extern int open(const char *path, int flags, ...);



#endif // fcntl_h___


