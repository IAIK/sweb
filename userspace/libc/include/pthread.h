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


#ifndef pthread_h___
#define pthread_h___

//pthread typedefs
typedef unsigned int pthread_t;
typedef unsigned int pthread_attr_t;

//pthread mutex typedefs
typedef unsigned int pthread_mutex_t;
typedef unsigned int pthread_mutexattr_t;

//pthread spinlock typedefs
#define PTHREAD_SPIN_T_DEFINED
typedef unsigned int pthread_spin_t;

//pthread cond typedefs
typedef unsigned int pthread_cond_t;
typedef unsigned int pthread_condattr_t;

/**
 * posix function signature
 * do not change the signature!
 */
extern int pthread_create(pthread_t *thread,
         const pthread_attr_t *attr, void *(*start_routine)(void *),
         void *arg);

/**
 * posix function signature
 * do not change the signature!
 */
extern void pthread_exit(void *value_ptr);

/**
 * posix function signature
 * do not change the signature!
 */
extern int pthread_cancel(pthread_t thread);

/**
 * posix function signature
 * do not change the signature!
 */
extern int pthread_join(pthread_t thread, void **value_ptr);

/**
 * posix function signature
 * do not change the signature!
 */
extern int pthread_mutex_init(pthread_mutex_t *mutex,
                              const pthread_mutexattr_t *attr);

/**
 * posix function signature
 * do not change the signature!
 */
extern int pthread_mutex_destroy(pthread_mutex_t *mutex);

/**
 * posix function signature
 * do not change the signature!
 */
extern int pthread_mutex_lock(pthread_mutex_t *mutex);

/**
 * posix function signature
 * do not change the signature!
 */
extern int pthread_mutex_unlock(pthread_mutex_t *mutex);

/**
 * posix function signature
 * do not change the signature!
 */
extern int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);

/**
 * posix function signature
 * do not change the signature!
 */
extern int pthread_cond_destroy(pthread_cond_t *cond);

/**
 * posix function signature
 * do not change the signature!
 */
extern int pthread_cond_signal(pthread_cond_t *cond);

/**
 * posix function signature
 * do not change the signature!
 */
extern int pthread_cond_broadcast(pthread_cond_t *cond);

/**
 * posix function signature
 * do not change the signature!
 */
extern int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);

#endif // pthread_h___


