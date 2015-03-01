#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

#include "types.h"
#include "Lock.h"
class Thread;

/**
 * @class SpinLock
 *
 * WikiPedia about SpinLocks:
 * @url http://en.wikipedia.org/wiki/Spinlock
 * In software engineering, a spinlock is a lock where the thread simply waits in a loop ("spins")
 * repeatedly checking until the lock becomes available. This is also known as "busy waiting"
 * because the thread remains active but isn't performing a useful task.
 *
 * The SpinLock is not meant to be used in a context where the InterruptFlag is not set,
 * because as with any lock, no taskswitch can happen and deadlock would occur if acquiring the SpinLock should
 * not succeed in an IF==0 context.
 * Also, in sweb, the Spinlock uses yield()s instead of a simple do-nothing-loop
 */
class SpinLock : public Lock
{
  public:

    SpinLock(const char* name);

    /**
     * Acquire the spinlock.
     * @param debug_info Additional debug information
     */
    void acquire(const char* debug_info = 0);

    /**
    * Try to acquire the spinlock. If the spinlock is held by another thread at the moment,
    * this method instantly returns (and (of course) the spinlock has not been acquired).
    * @param debug_info Additional debug information
    * @return true in case the spinlock has been acquired
    * @return false in case the spinlock was held by another thread
    */
    bool acquireNonBlocking(const char* debug_info = 0);

    /**
     * Release the spinlock.
     * @param debug_info Additional debug information
     */
    void release(const char* debug_info = 0);

    /**
     * allows you to check if the SpinLock is set or not.
     * trust the return value only if the SpinLock can't be acquired or releases
     * when you're not locking. (= only use in atomic state)
     */
    bool isFree();


  private:
    /**
     * The basic spinlock is just a variable which is
     */
    size_t lock_;

    /**
     * Do not use the copy constructor of the spinlock!
     * It is set to private to prevent it.
     */
    SpinLock(SpinLock const &);
    SpinLock &operator=(SpinLock const&);
};

#endif
