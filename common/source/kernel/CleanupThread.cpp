#include "CleanupThread.h"
#include "Scheduler.h"

CleanupThread::CleanupThread() : Thread(0, "CleanupThread")
{
  state_ = Worker;
}

void CleanupThread::Run()
{
  while (1)
  {
    while (hasWork())
    {
      Scheduler::instance()->cleanupDeadThreads();
    }
    waitForNextJob();
  }
}

