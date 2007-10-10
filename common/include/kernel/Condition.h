/**
 * @file Condition.h
 */

#ifndef CONDITION__
#define CONDITION__

#include "util/List.h"
#include "Thread.h"
#include "Mutex.h"

/**
 * @class Condition For Conditionmanagement
 *Conditions are very important to achieve mutual exclusion,
 *without busy waiting
 *
 *We need a Mutex*, because the only sane and working way to protect the list
 *in the CV is with the very same lock, the threads using the CV use.
 *Extra lock inside the CV won't work -> deadlock possibility
 *mixing lock and switching of interrupts wont work -> irq during time I have lock
 *only way: interrupts off or same lock
 *and interrupts off we want to avoid
 */
class Condition
{
  public:

    /**
     *Constructor
     *A Conditon needs a Mutex for creation. Only one thread can acquire this Mutex
     *and enter the critical section
     *
     * @param lock the Mutex
     * @return Condtition instance
     */
    Condition ( Mutex *lock );

    /**
     *Destructor
     *Deletes the list of sleeping threads, which are waiting for the Mutex
     */
    ~Condition();

    /**
     *Only possible if the current Thread has acquired the Mutex.
     *The Thread is put on the list of sleepers, releases the Mutex and goes to sleep
     *Acquires the Mutex when waking up.
     */
    void wait();

    /**
     *Wakes up the first Thread on the sleepers list.
     *If the list is empty, signal is being lost.
     */
    void signal();

    /**
     *Wakes up all Threads on the sleepers list.
     *If the list is empty, signal is being lost.
     */
    void broadcast();

  private:
    List<Thread *> *sleepers_;
    Mutex *lock_;

};

#endif
