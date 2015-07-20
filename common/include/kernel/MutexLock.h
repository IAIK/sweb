#ifndef _MUTEXLOCK_H_
#define _MUTEXLOCK_H_

#include "types.h"

class Mutex;

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
    MutexLock(Mutex &m);

    /**
     *Constructor
     *acquires the mutex
     * @param m the Mutex of the class Mutex
     * @param b whether the Mutex should be acquired or we still are in an interrupt free context
     */
    MutexLock(Mutex &m, bool b);

    /**
     *Destructor
     *releases the mutex
     */
    ~MutexLock();

  private:

    /**
     *Copy Constructor, but private.
     *
     *Don't use it!!!
     */
    MutexLock(MutexLock const&);

    /**
     * = Operator for the class MutexLock
     * private, mustn't be used
     */
    MutexLock &operator=(MutexLock const&);

    Mutex &mutex_;
    bool use_mutex_;
};

#endif
