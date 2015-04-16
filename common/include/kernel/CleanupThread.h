#ifndef CLEANUPTHREAD_H_
#define CLEANUPTHREAD_H_

#include "Thread.h"

class CleanupThread : public Thread
{
  public:
    CleanupThread();
    virtual ~CleanupThread();
    virtual void Run();
};

#endif /* CLEANUPTHREAD_H_ */
