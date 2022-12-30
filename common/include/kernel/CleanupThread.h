#pragma once

#include "Thread.h"

class CleanupThread : public Thread
{
  public:
    CleanupThread();
    ~CleanupThread() override;

    void kill() override;
    void Run() override;
};
