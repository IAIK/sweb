#pragma once

#include "CleanupThread.h"
#include "IdleThread.h"
#include "SMP.h"
#include "SchedulerLock.h"
#include "Thread.h"

#include "ArchCommon.h"
#include "ArchCpuLocalStorage.h"
#include "ArchInterrupts.h"
#include "ArchMulticore.h"

#include "types.h"

#include "EASTL/atomic.h"
#include "EASTL/list.h"
#include "EASTL/set.h"
#include "EASTL/vector_multiset.h"

#include "debug.h"

class Thread;
class Mutex;
class SpinLock;
class Lock;
class PreemptProtect;

extern __cpu Thread* currentThread;
extern __cpu ArchThreadRegisters* currentThreadRegisters;

extern cpu_local IdleThread* idle_thread;

extern __cpu eastl::atomic<size_t> preempt_protect_count_;

class Scheduler
{
  public:
    static Scheduler *instance();
    static bool isInitialized();

    void addNewThread(Thread *thread);
    void sleep(bool yield = true);
    void wake(Thread *thread_to_wake);
    void yield();
    void printThreadList();
    void printStackTraces();
    void printLockingInformation();
    bool isSchedulingEnabled();
    bool isCurrentlyCleaningUp();
    void incCpuTimerTicks();
    size_t getCpuTimerTicks() const;

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
    friend class PreemptProtect;

    void cleanupDeadThreads();

    struct ThreadVruntimeLess
    {
        constexpr bool operator()(const Thread* lhs, const Thread* rhs) const
        {
            return lhs->vruntime < rhs->vruntime;
        }
    };

    // vector_multiset does not alloc on erase/insert !
    using ThreadList = eastl::vector_multiset<Thread*, ThreadVruntimeLess>;

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

    uint64 descheduleThread(Thread* t, uint64 deschedule_time);

    static Scheduler *instance_;

    SchedulerLock scheduler_lock_;

    volatile char* locked_at_ = nullptr;

    size_t ticks_;

    CleanupThread cleanup_thread_;

public:
    ThreadList threads_;

    eastl::atomic<size_t> num_threads;
    eastl::atomic<size_t> scheduler_lock_count_free;
    eastl::atomic<size_t> scheduler_lock_count_blocked;

    Thread* minVruntimeThread();
    Thread* maxVruntimeThread();
    uint64 updateVruntime(Thread* t, uint64 now);
    ThreadList::iterator setThreadVruntime(Thread* t, uint64 new_vruntime);
    ThreadList::iterator setThreadVruntime(Scheduler::ThreadList::iterator it, uint64 new_vruntime);
};

class PreemptProtect
{
public:
    PreemptProtect() :
        intr(false)
    {
        ++preempt_protect_count_;
        size_t cpu_id = SMP::currentCpuId();
        ((char*)ArchCommon::getFBPtr())[80*2 + cpu_id*2 + 1] = CONSOLECOLOR::WHITE | (CONSOLECOLOR::RED << 4);
    }

    ~PreemptProtect()
    {
        size_t cpu_id = SMP::currentCpuId();
        ((char*)ArchCommon::getFBPtr())[80*2 + cpu_id*2 + 1] = CONSOLECOLOR::WHITE | (CONSOLECOLOR::BLACK << 4);
        --preempt_protect_count_;
    }

private:
    WithInterrupts intr;
};
