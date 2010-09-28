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


#ifndef semaphore_h___
#define semaphore_h___

//semaphores typedefs
#ifndef SEM_T_DEFINED_
#define SEM_T_DEFINED_
typedef unsigned int sem_t;
#endif // SEM_T_DEFINED_

/**
 * posix method signature
 * do not change the signature!
 */
extern int sem_init(sem_t *sem, int pshared, unsigned value);

/**
 * posix method signature
 * do not change the signature!
 */
extern int sem_wait(sem_t *sem);

/**
 * posix method signature
 * do not change the signature!
 */
extern int sem_trywait(sem_t *sem);

/**
 * posix method signature
 * do not change the signature!
 */
extern int sem_destroy(sem_t *sem);

/**
 * posix method signature
 * do not change the signature!
 */
extern int sem_post(sem_t *sem);


#endif // semaphore_h___


