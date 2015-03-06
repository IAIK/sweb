#include "stdarg.h"
#include "kprintf.h"
#include "Console.h"
#include "Terminal.h"
#include "debug_bochs.h"
#include "ArchInterrupts.h"
#include "RingBuffer.h"
#include "Scheduler.h"
#include "assert.h"
#include "debug.h"
#include "ustringformat.h"

//it's more important to keep the messages that led to an error, instead of
//the ones following it, when the nosleep buffer gets full

RingBuffer<char> *nosleep_rb_;
Thread *flush_thread_;

void flushActiveConsole()
{
  assert(main_console);
  assert(nosleep_rb_);
  assert(ArchInterrupts::testIFSet());
  char c = 0;
  bool found = false;
  while (nosleep_rb_->get(c))
  {
    main_console->getActiveTerminal()->write(c);
    flush_thread_->jobDone();
    found = true;
  }
  if (!found)
  {
    // There are open jobs but nothing in the ring-buffer, maybe the buffer was
    // full at any time and a char has been discarded, lets just complete a job
    // to catch up again.
    flush_thread_->jobDone();
  }
}

class KprintfFlushingThread : public Thread
{
  public:

    KprintfFlushingThread() : Thread(0, "KprintfFlushingThread")
    {
      state_ = Worker;
    }

    virtual void Run()
    {
      while (true)
      {
        while (hasWork())
        {
          flushActiveConsole();
        }
        waitForNextJob();
      }
    }
};

void kprintf_init()
{
  nosleep_rb_ = new RingBuffer<char>(1024);
  debug(KPRINTF, "Adding Important kprintf Flush Thread\n");
  flush_thread_ = new KprintfFlushingThread();
  Scheduler::instance()->addNewThread(flush_thread_);
}

void kprintf_func(int ch, void *arg __attribute__((unused)))
{
  //check if atomar or not in current context
  if ((ArchInterrupts::testIFSet() && Scheduler::instance()->isSchedulingEnabled())
      || (main_console->areLocksFree() && main_console->getActiveTerminal()->isLockFree()))
  {
    main_console->getActiveTerminal()->write(ch);
  }
  else
  {
    nosleep_rb_->put(ch);
    flush_thread_->addJob();
  }
}

void kprintf(const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  kvprintf(fmt, kprintf_func, 0, 10, args);
  va_end(args);
}

void kprintfd_func(int ch, void *arg __attribute__((unused)))
{
  writeChar2Bochs((uint8) ch);
}

void kprintfd(const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  kvprintf(fmt, kprintfd_func, 0, 10, args);
  va_end(args);
}
