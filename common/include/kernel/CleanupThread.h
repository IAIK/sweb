/*
 * CleanupThread.h
 *
 */

#ifndef CLEANUPTHREAD_H_
#define CLEANUPTHREAD_H_

#include "Thread.h"

/**
 * @class CleanupThread
 */
class CleanupThread : public Thread
{
public:
  /**
   * Constructor
   * @return CleanupThread instance
   */
  CleanupThread();

  /**
   * calls cleanUpDeadThreads
   */
  virtual void Run();
};



#endif /* CLEANUPTHREAD_H_ */
