// Projectname: SWEB
// Simple operating system for educational purposes
//
// Copyright (C) 2011  Daniel Gruss
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


#ifndef nonstd_h___
#define nonstd_h___

#include "../../../common/include/kernel/syscall-definitions.h"

/**
 * Creates a new process.
 *
 * @param path the path to the binary to open
 * @param sleep whether the calling process should sleep until the other process terminated
 * @return -1 if the path did not lead to an executable, 0 if the process was executed successfully
 *
 */ 
extern int createprocess(const char* path, int sleep);

#endif // nonstd_h___


