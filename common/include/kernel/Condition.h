#ifndef CONDITION__
#define CONDITION__

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
     *
     */
    void wait(const char* debug_info = 0, bool re_acquire_mutex = true);

    /**
     * Wakes up the first Thread on the sleepers list.
     * If the list is empty, signal is being lost.
     */
    void signal(const char* debug_info = 0);

    /**
     * Wakes up all Threads on the sleepers list.
     * If the list is empty, signal is being lost.
     */
    void broadcast(const char* debug_info = 0);

  private:
    /**
     * The mutex which is bound to this condition.
     */
    Mutex *mutex_;

};

#endif
