#include "CleanupThread.h"
#include "Scheduler.h"

CleanupThread::CleanupThread() :
    Thread("CleanupThread")
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

