#pragma once
#include "types.h"

class BasicSpinLock
{
public:
    BasicSpinLock();

    void acquire(bool yield = true);
    bool acquireNonBlocking();

    void release();

    bool isFree() const;
private:
    unsigned int lock_;
};
