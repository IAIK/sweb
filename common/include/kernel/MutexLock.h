#pragma once

#include "types.h"
#include "backtrace.h"

class Mutex;

class MutexLock
{
  public:
    MutexLock(Mutex& m, bool b = true, pointer called_by = getCalledBefore(0));
    ~MutexLock();

    MutexLock(const MutexLock&) = delete;
    MutexLock& operator=(const MutexLock&) = delete;

  private:

    Mutex& mutex_;
    bool use_mutex_;
};
