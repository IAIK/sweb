// Projectname: SWEB
// Simple operating system for educational purposes
//
// Copyright (C) 2010  Daniel Gruss, Matthias Reischer
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


#ifndef mman_h___
#define mman_h___


#define PROT_NONE     0x00000000  // 00..00
#define PROT_READ     0x00000001  // ..0001
#define PROT_WRITE    0x00000002  // ..0010

#define MAP_PRIVATE   0x00000000  // 00..00
#define MAP_SHARED    0x40000000  // 0100..
#define MAP_ANONYMOUS 0x80000000  // 1000..


/**
 * Used for sizes of objects
 *
 */
#ifndef SIZE_T_DEFINED
#define SIZE_T_DEFINED
typedef unsigned int size_t;
#endif // SIZE_T_DEFINED

/**
 * Used for shared mem modes
 *
 */
#ifndef MODE_T_DEFINED
#define MODE_T_DEFINED
typedef unsigned int mode_t;
#endif // MODE_T_DEFINED

/**
 * Used for setting offsets
 *
 */
#ifndef OFF_T_DEFINED
#define OFF_T_DEFINED
typedef unsigned int off_t;
#endif // OFF_T_DEFINED

/**
 * posix method signature
 * do not change the signature!
 */
extern void* mmap(void* start, size_t length, int prot, int flags, int fd, off_t offset);

/**
 * posix method signature
 * do not change the signature!
 */
extern int munmap(void* start, size_t length);

/**
 * posix method signature
 * do not change the signature!
 */
extern int shm_open(const char* name, int oflag, mode_t mode);

/**
 * posix method signature
 * do not change the signature!
 */
extern int shm_unlink(const char* name);


#endif // mman_h___


