#pragma once

#include "CpuExclusiveLock.h"

class Thread;

class SchedulerLock : public CpuExclusiveLock
{
public:
    SchedulerLock();

    SchedulerLock(const SchedulerLock&) = delete;

    virtual void acquire(pointer called_by = 0);
    virtual void release(pointer called_by = 0);
private:

    volatile Thread* scheduling_locked_by_ = nullptr;
};
