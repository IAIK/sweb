#pragma once

#include "Lock.h"

class Thread;
class Mutex;

/**
 * @class Condition For Condition management
 * Conditions are very important to achieve mutual exclusion,
 * without busy waiting
 *
 * We need a Mutex*, because the only sane and working way to protect the list
 * in the CV is with the very same lock, the threads using the CV use.
 * Extra lock inside the CV won't work -> deadlock possibility
 * mixing lock and switching of interrupts wont work -> irq during time I have lock
 * only way: interrupts off or same lock
 * and interrupts off we want to avoid
 */
class Condition : public Lock
{
  public:
    /**
   * Constructor
   * A Conditon needs a Mutex for creation. Only one thread can acquire this Mutex
   * and enter the critical section
   * @param mutex the Mutex
   * @param name the name of the condition
   * @return Condtition instance
   */
  Condition ( Mutex *mutex, const char* name);

    /**
     * Only possible if the current Thread has acquired the Mutex.
     * The Thread is put on the list of sleepers, releases the Mutex and goes to sleep
     * Acquires the Mutex when waking up.
     * @param re_acquire_mutex Automatically re-acquire the mutex after the wake-up.
     *        In case the mutex (or even the condition) does not exists any longer after wake-up,
     *        this flag has to be set to false. Then the mutex is not accessed any longer after wake up.
     * @param called_by A pointer to the call point of this function.
     *                  Can be set in case this method is called by a wrapper function.
     *
     */
    void wait(bool re_acquire_mutex = true, pointer called_by = 0);
    void waitAndRelease(pointer called_by = 0);

    /**
     * Wakes up the first Thread on the sleepers list.
     * If the list is empty, signal is being lost.
     * @param called_by A pointer to the call point of this function.
     *                  Can be set in case this method is called by a wrapper function.
     */
    void signal(pointer called_by = 0, bool broadcast = false);

    /**
     * Wakes up all Threads on the sleepers list.
     * If the list is empty, signal is being lost.
     * @param called_by A pointer to the call point of this function.
     *                  Can be set in case this method is called by a wrapper function.
     */
    void broadcast(pointer called_by = 0);

  private:
    /**
     * The mutex which is bound to this condition.
     */
    Mutex *mutex_;

};

