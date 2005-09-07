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
 * CVS Log Info for $RCSfile: read.c,v $
 *
 * $Id: read.c,v 1.1 2005/09/07 03:49:45 aniederl Exp $
 * $Log$
 *
 */


#include <unistd.h>

//----------------------------------------------------------------------
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
__syscall_3(off_t, lseek, int, file_descriptor, off_t, offset, int, whence)



//----------------------------------------------------------------------
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
__syscall_3(ssize_t, read, int, file_descriptor, void *, buffer, size_t, count)


