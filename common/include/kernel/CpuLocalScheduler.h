#pragma once
#include "Thread.h"
#include "ArchThreads.h"

class CpuLocalScheduler
{
public:
        void schedule();

        Thread* getCurrentThread();
        ArchThreadRegisters* getCurrentThreadRegisters();
        void setCurrentThread(Thread*);
        void setCurrentThreadRegisters(ArchThreadRegisters*);

private:
        ArchThreadRegisters* currentThreadRegisters_;
        Thread* currentThread_;
};
