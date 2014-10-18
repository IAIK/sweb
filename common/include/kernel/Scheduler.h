/**
 * @file Scheduler.h
 */

#ifndef SCHEDULER_H__
#define SCHEDULER_H__

#include "types.h"
#include "SpinLock.h"
#include "Mutex.h"
#include <ustl/ulist.h>

class Thread;
class ArchThreadInfo;

extern ArchThreadInfo *currentThreadInfo;
extern Thread *currentThread;


/**
 * @class Scheduler
 *
 * This is a singelton class, it is instantiated in startup() and must be accessed via Scheduler::instance()->....
 * The Scheduler knows about all running and sleeping threads and decides which thread to run next
 */
class Scheduler
{
  public:

    /**
     * Singelton Class Instance Access Method
     * @return Pointer to Scheduler
     */
    static Scheduler *instance();

    /**
     * createScheduler is called by startup() and does exatly what it's name implies.
     */
    static void createScheduler();

    /**
     * adds a new Thread and prepares to run it
     * this is the method that should be used to start a new Thread
     * @param *thread Pointer to the instance of a Class derived from Thread that contains the Thread to be started
     */
    void addNewThread ( Thread *thread );

    /**
     * removes the currently active Thread from the scheduling list
     * this method has actually no use right now and should propably not be used
     * if you want to keep a thread from being scheduled use sleep() instead
     * if you want to remove a thread permanently use Thread::kill() instead
     * you can't remove the last thread
     */
    void removeCurrentThread();

    /**
     * puts the currentThread to sleep and keeps it from being scheduled
     */
    void sleep();

    /**
     * wakes up a sleeping thread
     * @param *thread_to_wake, Pointer to the Thread that will be woken up
     */
    void wake ( Thread *thread_to_wake );

    /**
     * forces a task switch without waiting for the next timer interrupt
     */
    void yield();

    /**
     * prints a List of all Threads using kprintfd
     */
    void printThreadList();

    /**
     * prints a stack trace for each thread
     */
    void printStackTraces();

    /**
     * it is somewhat of a hack, we need to release the Spinlock,
     * after we set the ThreadState Sleeping, but before we yield away
     * also we must not be interrupted and we want to avoid disabling Interrupts
     * (even though it would be possible in this case, as we don't allocate memory)
     * @param &lock The SpinLock we want to release
     */
    void sleepAndRelease ( SpinLock &lock );

    /**
     * Overloading for Mutex
     * @param &lock the Mutex we want to release
     */
    void sleepAndRelease ( Mutex &lock );

    /**
     * yet another hack. Nelle's BD Driver needs to switch off Interrupts
     * and put Threads to sleep.
     * In order to avoid getting a wakeup before we put the Thread to sleep
     * we need to first sleep and _then_ enable Interrupts
     * Ohhh the Pain !!!
     * Everybody else using this function in any other sychronisation mechanism
     * where disabling Interrupts is not _absolutely_ neccessary is going to be lashed !
     */
    void sleepAndRestoreInterrupts ( bool interrupts );

    /**
     * compares all threads in the scheduler's list to the one given
     * since the scheduler knows about all existing threads, this is
     * a good way to see if a *thread is valid
     * @param *thread Pointer to the Thread's instance we want to check its existance bevore accessing it
     * @return true if *thread exists, fales otherwise
     */
    bool checkThreadExists ( Thread* thread );

    /**
     * @ret true if Scheduling is enabled, false otherwis
     */
    bool isSchedulingEnabled();

    /**
     * NEVER EVER EVER CALL THIS METHOD OUTSIDE OF AN INTERRUPT CONTEXT
     * this is the methode that decides which threads will be scheduled next
     * it is called by either the timer interrupt handler or the yield interrupt handler
     * and changes the global variables currentThread and currentThreadInfo
     * @return 1 if the InterruptHandler should switch to Usercontext or 0 if we can stay in Kernelcontext
     */
    uint32 schedule();

    /**
     * increments the stored ticks value by 1
     */
    void incTicks();

  protected:
    friend class IdleThread;
    /**
     * this method is periodically called by the idle-Thread
     * it removes and deletes Threads in state ToBeDestroyed
     */
    void cleanupDeadThreads();

    /**
     * returns the ticks value stored
     */
    uint32 getTicks();
    
  private:

    Scheduler();

    /**
     * Scheduler internal lock abstraction method
     * locks the thread-list against concurrent access by prohibiting a thread switch
     * don't call this from an Interrupt-Handler, since Atomicity won't be guaranteed
     */
    void lockScheduling();

    /**
     * Scheduler internal lock abstraction method
     * unlocks the thread-list
     */
    void unlockScheduling();

    /**
     * Scheduler internal lock abstraction method
     * tests the thread-list-lock without setting it,
     * use this _only_ in InterruptHandler-Context
     * @return true if lock is set, false otherwise
     */
    bool testLock();

    /**
     * after blocking the Scheduler we need to test if all Locks we could possible
     * acquire are really free, because otherwise we will deadlock.
     */
    void waitForFreeSpinLock(SpinLock& lock);

    static Scheduler *instance_;

    typedef ustl::list<Thread*> ThreadList;
    ThreadList threads_;

    size_t block_scheduling_;

    size_t ticks_;
};
#endif
