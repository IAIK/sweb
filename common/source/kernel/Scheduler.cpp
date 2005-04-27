//----------------------------------------------------------------------
//   $Id: Scheduler.cpp,v 1.2 2005/04/27 08:58:16 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: Scheduler.cpp,v $
//  Revision 1.1  2005/04/26 15:58:45  nomenquis
//  threads, scheduler, happy day
//
//----------------------------------------------------------------------


#include "Scheduler.h"
#include "Thread.h"
#include "arch_panic.h"
#include "ArchThreads.h"
#include "console/kprintf.h"

ArchThreadInfo *currentThreadInfo;
Thread *currentThread;

 
Scheduler *Scheduler::instance_=0;

Scheduler *Scheduler::instance()
{
  return instance_;
}

class IdleThread : public Thread
{
public:
  
  virtual void Run()
  {
    while (1)
    {
      kprintf("IDLE\n");
      Scheduler::instance()->yield();
    }
  }
};
void Scheduler::createScheduler()
{
  instance_ = new Scheduler();
  
  // create idle thread, this one really does not do too much

  Thread *idle = new IdleThread();
  instance_->addNewThread(idle);
}

Scheduler::Scheduler()
{
  uint32 i;
  for (i=0;i<MAX_THREADS;++i)
    threads_[i] = 0;
}

void Scheduler::addNewThread(Thread *thread)
{
  uint32 i;
  for (i=0;i<MAX_THREADS;++i)
  {
    if (!threads_[i])
    {
      threads_[i] = thread;
      break;
    }
  }
  if (i==MAX_THREADS)
  {
    arch_panic((uint8*)"Too many threads\n");
  }
  
  ArchThreads::createThreadInfosKernelThread(thread->arch_thread_info_,(pointer)&Scheduler::startThreadHack,thread->getStackStartPointer());
}

void Scheduler::startThreadHack()
{
  currentThread->Run();
}

void Scheduler::schedule(uint32 from_interrupt)
{
  uint32 i;
  for (i=0;i<MAX_THREADS;++i)
  {
    if (currentThread == threads_[i])
    {
      break;
    }
  }
  uint32 found = 0;
  while (!found)
  {
    i = (i+1)%MAX_THREADS;
    if (threads_[i])
      break;
  }
  
  currentThread = threads_[i];
  currentThreadInfo = threads_[i]->arch_thread_info_;
}

void Scheduler::yield()
{
  ArchThreads::yield();
}
