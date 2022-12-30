#pragma once

#include "Thread.h"

class IdleThread : public Thread
{
  public:
    IdleThread();
    ~IdleThread() override;

    void Run() override;
};
