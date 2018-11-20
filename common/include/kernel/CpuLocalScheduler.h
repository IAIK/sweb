#pragma once
#include "Thread.h"
#include "ArchThreads.h"
#include "Scheduler.h"
#include "Mutex.h"

/*
class CpuLocalScheduler
{
public:
        CpuLocalScheduler();

        void addNewThread(Thread* thread);

        void schedule();

        Thread* getCurrentThread();
        ArchThreadRegisters* getCurrentThreadRegisters();
        void setCurrentThread(Thread*);
        void setCurrentThreadRegisters(ArchThreadRegisters*);

        void cleanupDeadThreads();
        bool isCurrentlyCleaningUp();

        void lockThreadList();
        bool tryLockThreadList();
        void unlockThreadList();

        void incTicks();
        size_t getTicks();

private:
        //ArchThreadRegisters* currentThreadRegisters_;
        //Thread* currentThread_;

        Scheduler::ThreadList threads_;
        size_t thread_list_lock_ = 0;

        size_t ticks_;

        IdleThread idle_thread_;
        CleanupThread cleanup_thread_;
};
*/
