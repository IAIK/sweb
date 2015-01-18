/*
 * IdleThread.h
 *
 */

#ifndef IDLETHREAD_H_
#define IDLETHREAD_H_

#include "Thread.h"

/**
 * @class IdleThread
 */
class IdleThread : public Thread
{
  public:
    /**
     * Constructor
     * @return IdleThread instance
     */
    IdleThread();

    /**
     * Sets the system idle
     */
    virtual void Run();
};

#endif /* IDLETHREAD_H_ */
