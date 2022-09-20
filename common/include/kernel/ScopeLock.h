#pragma once

#include "types.h"

class Mutex;

class ScopeLock
{
  public:
    ScopeLock(Mutex &m);
    ScopeLock(Mutex &m, bool b);
    ~ScopeLock();

    ScopeLock(ScopeLock const&) = delete;
    ScopeLock &operator=(ScopeLock const&) = delete;

  private:

    Mutex &mutex_;
    bool use_mutex_;
};

