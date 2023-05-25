#pragma once

#include "EASTL/atomic.h"

class Thread;

class BasicSpinLock
{
public:
    BasicSpinLock();
    BasicSpinLock(const BasicSpinLock&) = delete;
    BasicSpinLock& operator=(const BasicSpinLock&) = delete;

    virtual ~BasicSpinLock() = default;

    void acquire(bool yield = true);
    bool acquireNonBlocking();

    void release();

    Thread* heldBy();
    bool isHeldBy(Thread* t);

protected:
    eastl::atomic_flag lock_;
    Thread* held_by_ = nullptr;
};
