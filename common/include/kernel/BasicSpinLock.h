#pragma once
#include "types.h"
#include "uatomic.h"

class Thread;

class BasicSpinLock
{
public:
    BasicSpinLock();
    BasicSpinLock(const BasicSpinLock&) = delete;

    ~BasicSpinLock() = default;

    void acquire(bool yield = true);
    bool acquireNonBlocking();

    void release();

    Thread* heldBy();

protected:
    ustl::atomic_flag lock_ = ATOMIC_FLAG_INIT;
    Thread* held_by_ = nullptr;
};
