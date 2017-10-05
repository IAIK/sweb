#pragma once

#include "types.h"

class Mutex;

class MutexLock
{
  public:
    MutexLock(Mutex &m);
    MutexLock(Mutex &m, bool b);
    ~MutexLock();

  private:
    MutexLock(MutexLock const&);
    MutexLock &operator=(MutexLock const&);

    Mutex &mutex_;
    bool use_mutex_;
};

