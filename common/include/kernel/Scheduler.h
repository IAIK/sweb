#pragma once

#include "types.h"
#include <ulist.h>
#include "IdleThread.h"
#include "CleanupThread.h"
#include <uatomic.h>

class Thread;
class Mutex;
class SpinLock;
class Lock;

extern __thread Thread* currentThread;
extern __thread ArchThreadRegisters* currentThreadRegisters;

extern __thread char scheduling_stack[PAGE_SIZE];

class Scheduler
{
  public:
    static Scheduler *instance();
    static bool isInitialized();

    void addNewThread(Thread *thread);
    void sleep();
    void wake(Thread *thread_to_wake);
    void yield();
    void printThreadList();
    void printStackTraces();
    void printLockingInformation();
    bool isSchedulingEnabled();
    bool isCurrentlyCleaningUp();
    void incTicks();
    uint32 getTicks();

    /**
     * NEVER EVER EVER CALL THIS METHOD OUTSIDE OF AN INTERRUPT CONTEXT
     * this is the method that decides which threads will be scheduled next
     * it is called by either the timer interrupt handler or the yield interrupt handler
     * and changes the global variables currentThread and currentThreadRegisters
     * @return 1 if the InterruptHandler should switch to Usercontext or 0 if we can stay in Kernelcontext
     */
    uint32 schedule();

  protected:
    friend class IdleThread;
    friend class CleanupThread;
    friend class CpuLocalScheduler;

    void cleanupDeadThreads();

    typedef ustl::list<Thread*> ThreadList;

  private:
    Scheduler();

    bool tryLockScheduling(const char* called_at);

    /**
     * Scheduler internal lock abstraction method
     * locks the thread-list against concurrent access by prohibiting a thread switch
     * don't call this from an Interrupt-Handler, since Atomicity won't be guaranteed
     */
    void lockScheduling(const char* called_at);

    /**
     * Scheduler internal lock abstraction method
     * unlocks the thread-list
     */
    void unlockScheduling(const char* called_at);

    static Scheduler *instance_;


    ustl::atomic<size_t> block_scheduling_;
    volatile Thread* scheduling_blocked_by_ = nullptr;
    volatile char* locked_at_ = nullptr;
    volatile size_t locked_by_cpu_ = -1;

    size_t ticks_;

    CleanupThread cleanup_thread_;

public:
    ThreadList threads_;

    size_t schedulable_threads;
};
