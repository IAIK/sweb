#pragma once

#include "CpuExclusiveLock.h"

class Thread;

class SchedulerLock : public CpuExclusiveLock
{
public:
    SchedulerLock();

    SchedulerLock(const SchedulerLock&) = delete;

    void acquire(pointer called_by = 0) override;
    void release(pointer called_by = 0) override;
private:

    volatile Thread* scheduling_locked_by_ = nullptr;
};
