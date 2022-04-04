#pragma once

#include "types.h"

class Mutex;

class MutexLock
{
  public:
    MutexLock(Mutex &m);
    MutexLock(Mutex &m, bool b);
    ~MutexLock();

    MutexLock(MutexLock const&) = delete;
    MutexLock &operator=(MutexLock const&) = delete;

  private:

    Mutex &mutex_;
    bool use_mutex_;
};

