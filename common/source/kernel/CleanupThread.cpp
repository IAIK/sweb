#include "CleanupThread.h"
#include "Scheduler.h"

CleanupThread::CleanupThread() : Thread(0, "CleanupThread")
{
  state_ = Worker;
}

CleanupThread::~CleanupThread()
{
  assert(false && "CleanupThread destruction means that you have accessed an invalid pointer somewhere.");
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

