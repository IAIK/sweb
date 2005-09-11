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
 * CVS Log Info for $RCSfile: brk.c,v $
 *
 * $Id: brk.c,v 1.2 2005/09/11 13:12:50 aniederl Exp $
 * $Log: brk.c,v $
 * Revision 1.1  2005/09/11 12:31:18  aniederl
 * initial import of brk
 *
 */


#include "unistd.h"
#include "sys/syscall.h"

/**
 * Marks the current break, set by brk() and used by sbrk()
 *
 */
void *__current_break = NULL;

__syscall_special_1(void *, __syscall_brk, void *, data_segment_end)

//----------------------------------------------------------------------
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
int brk(void *data_segment_end)
{
  // inspired from uClibc

  __current_break = __syscall_brk(data_segment_end);

  // out of memory
  if(__current_break < data_segment_end)
    return -1;

  return 0;
}

//----------------------------------------------------------------------
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
void *sbrk(intptr_t increment)
{
  // inspired from uClibc

  void *old_break;

  if(!__current_break)
  {
    // Initialize
    if(brk(NULL) < 0)
      return (void *) -1;
  }

  if(!increment)
    return __current_break;

  old_break = __current_break;

  if(brk(old_break + increment) < 0)
    return (void *) -1;

  return old_break;
}
