#include "Thread.h"
#include "ArchCommon.h"
#include "kprintf.h"
#include "ArchThreads.h"
#include "ArchInterrupts.h"
#include "Scheduler.h"
#include "Loader.h"
#include "Console.h"
#include "Terminal.h"
#include "backtrace.h"
#include "KernelMemoryManager.h"
#include "Stabs2DebugInfo.h"

#define MAX_STACK_FRAMES 20

const char* Thread::threadStatePrintable[4] =
{
"Running", "Sleeping", "ToBeDestroyed", "Worker"
};

extern "C" void threadStartHack()
{
  currentThread->setTerminal(main_console->getActiveTerminal());
  currentThread->Run();
  currentThread->kill();
  debug(THREAD, "ThreadStartHack: Panic, thread couldn't be killed\n");
  while(1);
}

Thread::Thread(FileSystemInfo *working_dir, ustl::string name) :
    kernel_registers_(0), user_registers_(0), switch_to_userspace_(0), loader_(0), state_(Running),
    next_thread_in_lock_waiters_list_(0), lock_waiting_on_(0), holding_lock_list_(0), tid_(0),
    my_terminal_(0), working_dir_(working_dir), name_(name)
{
  debug(THREAD, "Thread ctor, this is %p, stack is %p, fs_info ptr: %p\n", this, kernel_stack_, working_dir_);
  ArchThreads::createKernelRegisters(kernel_registers_, (void*)threadStartHack, getStackStartPointer());
  kernel_stack_[2047] = STACK_CANARY;
  kernel_stack_[0] = STACK_CANARY;
}

Thread::~Thread()
{
  debug(THREAD, "~Thread: freeing ThreadInfos\n");
  delete user_registers_;
  user_registers_ = 0;
  delete kernel_registers_;
  kernel_registers_ = 0;
  if(unlikely(holding_lock_list_ != 0))
  {
    debug(THREAD, "~Thread: ERROR: Thread <%s (%p)> is going to be destroyed, but still holds some locks!\n",
          getName(), this);
    Lock::printHoldingList(this);
    assert(false);
  }
  Scheduler::instance()->printStackTraces();
  debug(THREAD, "~Thread: done (%s)\n", name_.c_str());
}

//if the Thread we want to kill, is the currentThread, we better not return
// DO Not use new / delete in this Method, as it sometimes called from an Interrupt Handler with Interrupts disabled
void Thread::kill()
{
  debug(THREAD, "kill: Called by <%s (%p)>. Preparing Thread <%s (%p)> for destruction\n", currentThread->getName(),
        currentThread, getName(), this);

  switch_to_userspace_ = 0;

  Scheduler::instance()->invokeCleanup();
  state_ = ToBeDestroyed;

  if (currentThread == this)
  {
    ArchInterrupts::enableInterrupts();
    Scheduler::instance()->yield();
  }
}

void* Thread::getStackStartPointer()
{
  pointer stack = (pointer) kernel_stack_;
  stack += sizeof(kernel_stack_) - sizeof(uint32);
  return (void*)stack;
}

bool Thread::isStackCanaryOK()
{
  return ((kernel_stack_[0] == STACK_CANARY) && (kernel_stack_[2047] == STACK_CANARY));
}

Terminal *Thread::getTerminal()
{
  if (my_terminal_)
    return my_terminal_;
  else
    return (main_console->getActiveTerminal());
}

void Thread::setTerminal(Terminal *my_term)
{
  my_terminal_ = my_term;
}

void Thread::printBacktrace()
{
  printBacktrace(currentThread != this);
}

FileSystemInfo* Thread::getWorkingDirInfo(void)
{
  return working_dir_;
}

void Thread::setWorkingDirInfo(FileSystemInfo* working_dir)
{
  working_dir_ = working_dir;
}

extern Stabs2DebugInfo const *kernel_debug_info;

void Thread::printBacktrace(bool use_stored_registers)
{
  if (!kernel_debug_info)
  {
    debug(BACKTRACE, "Kernel debug info not set up, backtrace won't look nice!\n");
  }

  pointer call_stack[MAX_STACK_FRAMES];
  size_t count = backtrace(call_stack, MAX_STACK_FRAMES, this, use_stored_registers);

  Stabs2DebugInfo const *deb = kernel_debug_info;

  debug(BACKTRACE, "=== Begin of backtrace for %s thread <%s> ===\n", user_registers_ ? "user" : "kernel", getName());
  for (size_t j = 0; j < 2; ++j)
  {
    for (size_t i = 0; i < count; ++i)
    {
      char function_name[256];
      pointer start_addr = 0;
      if (deb)
        start_addr = deb->getFunctionName(call_stack[i], function_name, 255);

      if (start_addr)
      {
        ssize_t line = deb->getFunctionLine(start_addr, call_stack[i] - start_addr);
        if (line > 0)
          debug(BACKTRACE, "    %02zu/%02zu  %10zx  %s:%zu\n", i + 1, count, call_stack[i], function_name, line);
        else
          debug(BACKTRACE, "    %02zu/%02zu  %10zx  %s+%zx\n", i + 1, count, call_stack[i], function_name,
                call_stack[i] - start_addr);
      }
      else
        debug(BACKTRACE, "    %02zu/%02zu  %10zx  <UNKNOWN FUNCTION>\n", i + 1, count, call_stack[i]);
    }

    if (!user_registers_ || deb != kernel_debug_info)
      break;
    debug(BACKTRACE, "    -----   Userspace  --------------------\n");
    count = backtrace_user(call_stack, MAX_STACK_FRAMES, this, 0);

    if (loader_)
      deb = loader_->getDebugInfos();
    else
      deb = 0;
  }

  debug(BACKTRACE, "=== End of backtrace for %s thread <%s> ===\n", user_registers_ ? "user" : "kernel", getName());
}

void Thread::addJob()
{
  if(!ArchInterrupts::testIFSet())
  {
    jobs_scheduled_++;
  }
  else
  {
    ArchThreads::atomic_add(jobs_scheduled_, 1);
  }
}

void Thread::jobDone()
{
  ArchThreads::atomic_add(jobs_done_, 1);
}

void Thread::waitForNextJob()
{
  assert(state_ == Worker);
  Scheduler::instance()->yield();
}

bool Thread::hasWork()
{
  return jobs_done_ < jobs_scheduled_;
}

bool Thread::isWorker() const
{
  // If it is a worker thread, the thread state may be sleeping.
  // But in this case, the thread has at least one job scheduled.
  return (state_ == Worker) || (jobs_scheduled_ > 0);
}

bool Thread::schedulable()
{
  return (state_ == Running) || (state_ == Worker && hasWork());
}

const char *Thread::getName()
{
  return name_.c_str();
}

size_t Thread::getTID()
{
  return tid_;
}
