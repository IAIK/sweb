#pragma once

#include "backtrace.h"
#include "types.h"

class Mutex;

class ScopeLock
{
public:
    ScopeLock(Mutex& m, bool b = true, pointer called_by = getCalledBefore(0));
    ~ScopeLock();

    ScopeLock(const ScopeLock&) = delete;
    ScopeLock& operator=(const ScopeLock&) = delete;

private:
    Mutex& mutex_;
    bool use_mutex_;
};
