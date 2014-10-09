#ifndef semaphore_h___
#define semaphore_h___

//semaphores typedefs
#ifndef SEM_T_DEFINED_
#define SEM_T_DEFINED_
typedef unsigned int sem_t;
#endif // SEM_T_DEFINED_

/**
 * posix function signature
 * do not change the signature!
 */
extern int sem_init(sem_t *sem, int pshared, unsigned value);

/**
 * posix function signature
 * do not change the signature!
 */
extern int sem_wait(sem_t *sem);

/**
 * posix function signature
 * do not change the signature!
 */
extern int sem_trywait(sem_t *sem);

/**
 * posix function signature
 * do not change the signature!
 */
extern int sem_destroy(sem_t *sem);

/**
 * posix function signature
 * do not change the signature!
 */
extern int sem_post(sem_t *sem);


#endif // semaphore_h___


