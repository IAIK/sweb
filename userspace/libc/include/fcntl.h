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
 * $Id: fcntl.h,v 1.4 2006/11/18 20:27:02 aniederl Exp $
 * $Log: fcntl.h,v $
 * Revision 1.3  2005/09/20 15:58:37  aniederl
 * included unistd.h
 *
 * Revision 1.2  2005/09/13 17:31:04  aniederl
 * removed "unnecessary" functions
 *
 * Revision 1.1  2005/09/11 13:15:46  aniederl
 * import of fcntl.h
 *
 */


#ifndef fcntl_h___
#define fcntl_h___

#include "unistd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Duplicate file descriptor
 *
 */
#define F_DUPFD 0x0000

/**
 * Get file descriptor flags
 *
 */
#define F_GETFD 0x0001

/**
 * Set file descriptor flags
 *
 */
#define F_SETFD 0x0002

/**
 * Get file status flags and file access mode
 *
 */
#define F_GETFL 0x0004

/**
 * Set file status flags
 *
 */
#define F_SETFL 0x0008

/**
 * Get record locking information
 *
 */
#define F_GETLK 0x0010

/**
 * Set record locking information
 *
 */
#define F_SETLK 0x0020

/**
 * Set record locking information; wait if blocked
 *
 */
#define F_SETLKW 0x0040

/**
 * Close the file descriptor upon execution of an exec family function
 *
 */
#define FD_CLOEXEC 0x0080

/**
 * Shared or read lock
 *
 */
#define F_RDLCK 0x0100

/**
 * Unlock
 *
 */
#define F_UNLCK 0x0200

/**
 * Exclusive or write lock
 *
 */
#define F_WRLCK 0x0400


/**
 * Open for reading only
 *
 */
#define O_RDONLY 0x0000

/**
 * Open for writing only
 *
 */
#define O_WRONLY 0x0001

/**
 * Open for reading and writing
 *
 */
#define O_RDWR 0x0002


/**
 * Create file if it does not exist
 *
 */
#define O_CREAT 0x0004

/**
 * Truncate flag
 *
 */
#define O_TRUNC 0x0008

/**
 * Set append mode
 *
 */
#define O_APPEND 0x0010

/**
 * Non-blocking mode
 *
 */
#define O_NONBLOCK 0x0020

/**
 * Synchronized read I/O operations
 *
 */
#define O_RSYNC 0x0040

/**
 * Mask for file access modes
 *
 */
#define O_ACCMODE 0x0080


/**
 * Readable flag
 *
 */
#define A_READABLE  0x0001

/**
 * Writeable flag
 *
 */
#define A_WRITABLE  0x0002

/**
 * Executable flag
 *
 */
#define A_EXECABLE  0x0004


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

#ifdef __cplusplus
}
#endif

#endif // fcntl_h___


