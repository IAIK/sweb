#include "CleanupThread.h"

#include "Scheduler.h"
#include "VgaColors.h"

#include "ArchMulticore.h"

#include "assert.h"

CleanupThread::CleanupThread() : Thread(0, "CleanupThread", Thread::KERNEL_THREAD)
{
    console_color = CONSOLECOLOR::BROWN;
}

CleanupThread::~CleanupThread()
{
  assert(false && "CleanupThread destruction means that you probably have accessed an invalid pointer somewhere.");
}

void CleanupThread::kill()
{
  assert(false && "CleanupThread destruction means that you probably have accessed an invalid pointer somewhere.");
}

void CleanupThread::Run()
{
  while (true)
  {
    Scheduler::instance()->cleanupDeadThreads();
    Scheduler::instance()->yield();
  }
}
