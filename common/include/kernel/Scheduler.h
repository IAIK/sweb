#pragma once

#include "types.h"
#include <ulist.h>
#include <umultiset.h>
#include "IdleThread.h"
#include "CleanupThread.h"
#include <uatomic.h>
#include "Thread.h"
#include "debug.h"

class Thread;
class Mutex;
class SpinLock;
class Lock;

extern __thread Thread* currentThread;
extern __thread ArchThreadRegisters* currentThreadRegisters;

extern thread_local IdleThread idle_thread;

extern __thread size_t cpu_ticks;

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
    void incCpuTicks();
    uint32 getTicks() const;
    uint32 getCpuTicks() const;

    /**
     * NEVER EVER EVER CALL THIS METHOD OUTSIDE OF AN INTERRUPT CONTEXT
     * this is the method that decides which threads will be scheduled next
     * it is called by either the timer interrupt handler or the yield interrupt handler
     * and changes the global variables currentThread and currentThreadRegisters
     */
    void schedule();

  protected:
    friend class IdleThread;
    friend class CleanupThread;
    friend class CpuLocalScheduler;

    void cleanupDeadThreads();

    struct ThreadVruntimeLess
    {
        constexpr bool operator()(const Thread* lhs, const Thread* rhs) const
        {
            return lhs->vruntime < rhs->vruntime;
        }
    };

    typedef ustl::multiset<Thread*, ThreadVruntimeLess> ThreadList;

  private:
    Scheduler();

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

    size_t ticks_;

    CleanupThread cleanup_thread_;

public:
    ThreadList threads_;

    ustl::atomic<size_t> num_threads;
    ustl::atomic<size_t> scheduler_lock_count_free;
    ustl::atomic<size_t> scheduler_lock_count_blocked;

    Thread* minVruntimeThread();
    Thread* maxVruntimeThread();
    void updateVruntime(Thread* t, uint64 now);
    void setThreadVruntime(Thread* t, uint64 new_vruntime);
    void setThreadVruntime(Scheduler::ThreadList::iterator it, uint64 new_vruntime);
};
