#include "CpuLocalScheduler.h"
#include "debug.h"

Thread* CpuLocalScheduler::getCurrentThread()
{
        return currentThread_;
}

ArchThreadRegisters* CpuLocalScheduler::getCurrentThreadRegisters()
{
        return currentThreadRegisters_;
}


void CpuLocalScheduler::setCurrentThread(Thread* t)
{
        debug(CPU_SCHEDULER, "setCurrentThread: %p\n", t);
        currentThread_ = t;
}

void CpuLocalScheduler::setCurrentThreadRegisters(ArchThreadRegisters* r)
{
        debug(CPU_SCHEDULER, "setCurrentThreadRegisters: %p\n", r);
        currentThreadRegisters_ = r;
}
