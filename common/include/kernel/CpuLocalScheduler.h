#pragma once
#include "Thread.h"
#include "ArchThreads.h"
#include "Scheduler.h"
#include "Mutex.h"

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

private:
        ArchThreadRegisters* currentThreadRegisters_;
        Thread* currentThread_;

        Scheduler::ThreadList threads_;
        Mutex thread_list_lock_;

        IdleThread idle_thread_;
        CleanupThread cleanup_thread_;
};
