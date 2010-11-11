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


#ifndef time_h___
#define time_h___

#define CLOCKS_PER_SEC 1000000

#ifndef CLOCK_T_DEFINED
#define CLOCK_T_DEFINED
typedef unsigned int clock_t;
#endif // CLOCK_T_DEFINED

/**
 * posix function signature
 * do not change the signature!
 */
extern clock_t clock(void);



#endif // time_h___


