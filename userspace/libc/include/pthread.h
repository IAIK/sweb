#ifndef pthread_h___
#define pthread_h___

//pthread typedefs
typedef unsigned int pthread_t;
typedef unsigned int pthread_attr_t;

//pthread mutex typedefs
typedef unsigned int pthread_mutex_t;
typedef unsigned int pthread_mutexattr_t;

//pthread spinlock typedefs
#define PTHREAD_SPINLOCK_T_DEFINED
typedef unsigned int pthread_spinlock_t;

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
#define PTHREAD_DETACH_DEFINED
extern int pthread_detach(pthread_t thread);

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


