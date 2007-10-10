/**
 * @file Mutex.h
 */
#ifndef _MUTEX_H_
#define _MUTEX_H_

#include "types.h"
#include "util/List.h"
#include "SpinLock.h"

class Thread;

/**
 * @class Mutex
 *This is intended to be your standard-from-the-shelf Lock
 *However, since we cannot turn off interrupts and allocate Memory
 *at the same time, we have to use some other means:

// In Sweb we face the following Problems in designing a Lock
// - we want to assure mutual exclusion
// - threads put to sleep need to be remembered -> put on a list -> means allocating memory
// - we can't switch of Interrupts while allocating memory
// - we need to lock the list which is used by the lock
//
// To lock the sleepers_ list, we use an even simpler Mutex called SpinLock
// SpinLock doesn't acquire memory but instead threads waiting on the spinlock
// just loop until the lock is free. Unfortunately there is no way around this,
// until threads can be put to sleep properly.
//
// Another Problem we face are RaceConditions between acquire and release.
// To avoid, that release() might take a thread from the list, before acquire() has
// put one there, we need to acquire the spinlock before actually checking the
// Mutex-Lock itself.
// Thus we can assure, that we are on the list, before release is run, but not that
// we are actually asleep, before release tries to wake us. (in which case, we would never be woken)
// Therefore we can release the SpinLock only after we have gone to sleep. (after a fashion,
// since doing anything after having gone to sleep, is of course Impossible)
// the Scheduler Method sleepAndRelease() tries to accomplish this trick for us.
 */
class Mutex
{
  public:

    /**
     *Constructor
     */
    Mutex();

    /**
     *acquire sets the Lock and must be called at the start of
     *a critical region, assuring that code between acquire and release,
     *can not be run by two threads at the same time.
     *Threads calling acquire on a already set lock, will be put to sleep
     */
    void acquire();

    /**
     *release frees the Lock. It must be called at the end of
     *a critical region, allowing other threads to execute code
     *in the critical region and to wake up the next sleeping thread.
     */
    void release();

    /**
     *allows us to check if the Lock is set or not.
     *this should be used only if you really really know what you are doing and
     *mutual exclusion is assured in some other way. Of course, if mutual exclusion
     *is assured in another way, why do you need to check a Lock in the first place ?
     *so there really is no good reason to use isFree() :-)
     *(you can check an example use in kprintf_nosleep)
     *
     * @return true if lock is set, false otherwise
     */
    bool isFree();

  protected:
    friend class Condition;

    /**
     *Returns if the lock is held by the Thread, that means,
     *if the Thread has created this lock
     *
     * @return true if lock is held by the given Thread
     */
    bool isHeldBy ( Thread *thread )
    {
      return ( held_by_==thread );
    }


  private:

    uint32 mutex_;
    List<Thread*> sleepers_;
    Thread *held_by_;
    SpinLock spinlock_;


    /**
     *Copy Constructor, but private.
     *
     *Don't use it!!!
     */
    Mutex ( Mutex const & ) {}

    /**
     * = Operator for the class Mutex
     * @return this
     */
    Mutex &operator= ( Mutex const& ) {return *this;}
};


/**
 * @class MutexLock holds the Mutex of the class Mutex
 */
class MutexLock
{
  public:

    /**
     *Constructor
     *acquires the mutex
     * @param m the Mutex of the class Mutex
     */
    MutexLock ( Mutex &m ) :mutex_ ( m )
    {
      mutex_.acquire();
    }

    /**
     *Destructor
     *releases the mutex
     */
    ~MutexLock()
    {
      mutex_.release();
    }

  private:

    /**
     *Copy Constructor, but private.
     *
     *Don't use it!!!
     */
    MutexLock ( MutexLock const& ) : mutex_ ( *static_cast<Mutex*> ( 0 ) )
    {}

    Mutex &mutex_;
};

#endif
