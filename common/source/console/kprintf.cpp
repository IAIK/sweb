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

class KprintfNoSleepFlushingThread : public Thread
{
  public:

    KprintfNoSleepFlushingThread() :
        Thread("KprintfNoSleepFlushingThread")
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
  flush_thread_ = new KprintfNoSleepFlushingThread();
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

bool isDebugEnabled(size_t flag)
{
  bool group_enabled = false;

  if (!(flag & OUTPUT_ENABLED))
  {
    size_t group_flag = flag & 0x7fff0000;
    group_flag |= OUTPUT_ENABLED;
    switch (group_flag)
    {
      case BD:
      case CONSOLE:
      case KERNEL:
      case MM:
      case VFSSYSCALL:
      case DRIVER:
      case ARCH:
        group_enabled = true;
        break;
    }
  }
  if ((flag & OUTPUT_ENABLED) || group_enabled)
  {
    return true;
  }
  return false;
}

#ifndef NO_COLOR
#define COLORDEBUG(str, color) "\033[0;%sm%s\033[1;m", color, str
#else
#define COLORDEBUG(str, color) str
#endif

void debug(size_t flag, const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  if (isDebugEnabled(flag))
  {
    switch (flag)
    {
      case M_INODE:
        kprintfd(COLORDEBUG("[M_INODE    ]", "33"));
        break;
      case M_STORAGE_MANAGER:
        kprintfd(COLORDEBUG("[M_STORAGE_M]", "33"));
        break;
      case M_SB:
        kprintfd(COLORDEBUG("[M_SB       ]", "33"));
        break;
      case M_ZONE:
        kprintfd(COLORDEBUG("[M_ZONE     ]", "33"));
        break;
      case BD_MANAGER:
        kprintfd(COLORDEBUG("[BD_MANAGER ]", "33"));
        break;
      case KPRINTF:
        kprintfd(COLORDEBUG("[KPRINTF    ]", "33"));
        break;
      case CONDITION:
        kprintfd(COLORDEBUG("[CONDITION  ]", "33"));
        break;
      case LOADER:
        kprintfd(COLORDEBUG("[LOADER     ]", "37"));
        break;
      case SCHEDULER:
        kprintfd(COLORDEBUG("[SCHEDULER  ]", "33"));
        break;
      case SYSCALL:
        kprintfd(COLORDEBUG("[SYSCALL    ]", "34"));
        break;
      case MAIN:
        kprintfd(COLORDEBUG("[MAIN       ]", "31"));
        break;
      case THREAD:
        kprintfd(COLORDEBUG("[THREAD     ]", "35"));
        break;
      case USERPROCESS:
        kprintfd(COLORDEBUG("[USERPROCESS]", "36"));
        break;
      case MOUNTMINIX:
        kprintfd(COLORDEBUG("[MOUNTMINIX ]", "36"));
        break;
      case BACKTRACE:
        kprintfd(COLORDEBUG("[BACKTRACE  ]", "31"));
        break;
      case USERTRACE:
        kprintfd(COLORDEBUG("[USERTRACE  ]", "31"));
        break;
      case PM:
        kprintfd(COLORDEBUG("[PM         ]", "32"));
        break;
      case KMM:
        kprintfd(COLORDEBUG("[KMM        ]", "33"));
        break;
      case ATA_DRIVER:
        kprintfd(COLORDEBUG("[ATA_DRIVER ]", "33"));
        break;
      case MMC_DRIVER:
        kprintfd(COLORDEBUG("[MMC_DRIVER ]", "33"));
        break;
      case IDE_DRIVER:
        kprintfd(COLORDEBUG("[IDE_DRIVER ]", "33"));
        break;
      case A_COMMON:
        kprintfd(COLORDEBUG("[A_COMMON   ]", "33"));
        break;
      case A_MEMORY:
        kprintfd(COLORDEBUG("[A_MEMORY   ]", "33"));
        break;
      case A_SERIALPORT:
        kprintfd(COLORDEBUG("[A_SERIALPRT]", "33"));
        break;
      case A_KB_MANAGER:
        kprintfd(COLORDEBUG("[A_KB_MANAGR]", "33"));
        break;
      case BD_VIRT_DEVICE:
        kprintfd(COLORDEBUG("[BD_VIRT_DEV]", "33"));
        break;
      case A_INTERRUPTS:
        kprintfd(COLORDEBUG("[A_INTERRUPT]", "33"));
        break;
      case VFSSYSCALL:
        kprintfd(COLORDEBUG("[VFSSYSCALL ]", "33"));
        break;
      case RAMFS:
        kprintfd(COLORDEBUG("[RAMFS      ]", "37"));
        break;
      case DENTRY:
        kprintfd(COLORDEBUG("[DENTRY     ]", "38"));
        break;
      case PATHWALKER:
        kprintfd(COLORDEBUG("[PATHWALKER ]", "33"));
        break;
      case PSEUDOFS:
        kprintfd(COLORDEBUG("[PSEUDOFS   ]", "33"));
        break;
      case VFS:
        kprintfd(COLORDEBUG("[VFS        ]", "33"));
        break;
    }
    kvprintf(fmt, kprintfd_func, 0, 10, args);
  }

  va_end(args);
}
