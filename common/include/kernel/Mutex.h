#ifndef _MUTEX_H_
#define _MUTEX_H_

#include "types.h"
#include "MutexLock.h"
#include "Lock.h"
class Thread;

/**
 * @class Mutex
 * This is intended to be your standard-from-the-shelf Lock.
 * When a thread is not able to directly acquire a mutex,
 * it puts itself onto the waiters list and goes to sleep.
 * Whenever a thread holding the mutex is going to release it,
 * it wakes up a thread waiting on this mutex.
 */
class Mutex: public Lock
{
  friend class Scheduler;
  friend class Condition;

public:

  Mutex(const char* name);

  /**
   * like acquire, but instead of blocking the currentThread until the Lock is free
   * acquireNonBlocking() immediately returns True or False, depending on whether the Lock
   * was acquired successfully or already held by another Thread.
   */
  bool acquireNonBlocking(const char* debug_info = (const char*)0);

  /**
   * acquire sets the Lock and must be called at the start of
   * a critical region, assuring that code between acquire and release,
   * can not be run by two threads at the same time.
   * Threads calling acquire on a already set lock, will be put to sleep
   */
  void acquire(const char* debug_info = (const char*)0);

  /**
   * release frees the Lock. It must be called at the end of
   * a critical region, allowing other threads to execute code
   * in the critical region and to wake up the next sleeping thread.
   */
  void release(const char* debug_info = (const char*)0);

  /**
   * allows us to check if the Lock is set or not.
   * this should be used only if you really really know what you are doing and
   * mutual exclusion is assured in some other way. Of course, if mutual exclusion
   * is assured in another way, why do you need to check a Lock in the first place ?
   * so there really is no good reason to use isFree() :-)
   *
   * @return true if lock is set, false otherwise
   */
  bool isFree();

private:

  /**
   * The basic mutex.
   * It is atomic set to 1 when acquired,
   * and set to 0 when the mutex is released.
   */
  size_t mutex_;

  /**
   * Copy Constructor, but private.
   *
   * Don't use it!!!
   */
  Mutex(Mutex const &);

  /**
   * = Operator for the class Mutex
   * private, mustn't be used
   */
  Mutex &operator=(Mutex const&);

};

#endif
