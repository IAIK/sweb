#ifndef IDLETHREAD_H_
#define IDLETHREAD_H_

#include "Thread.h"

class IdleThread : public Thread
{
  public:
    IdleThread();

    virtual void Run();
};

#endif /* IDLETHREAD_H_ */
