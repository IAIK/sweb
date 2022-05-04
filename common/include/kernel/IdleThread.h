#pragma once

#include "Thread.h"

class IdleThread : public Thread
{
  public:
    IdleThread();
    virtual ~IdleThread();

    virtual void Run();
};
