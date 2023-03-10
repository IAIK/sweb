#include "Thread.h"

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
#include "ProcessRegistry.h"

static constexpr size_t BACKTRACE_MAX_FRAMES = 20;

extern "C" [[noreturn]] void threadStartHack()
{
  assert(currentThread);
  currentThread->setTerminal(main_console->getActiveTerminal());
  currentThread->Run();
  currentThread->kill();
  debugAlways(THREAD, "ThreadStartHack: Panic, thread couldn't be killed\n");
  assert(false);
}

Thread::Thread(FileSystemInfo* working_dir, eastl::string name, Thread::TYPE type) :
    kernel_registers_(ArchThreads::createKernelRegisters(
        (type == Thread::USER_THREAD ? nullptr : (void*)&threadStartHack),
        getKernelStackStartPointer())),
    switch_to_userspace_(type == Thread::USER_THREAD ? 1 : 0),
    console_color(CONSOLECOLOR::BRIGHT_BLUE),
    state_(Running),
    tid_(0),
    working_dir_(working_dir),
    name_(eastl::move(name))
{
    debug(THREAD,
          "Thread ctor, this is %p, name: %s, kernel stack: [%p, %p), working_dir ptr: "
          "%p\n",
          this, getName(), kernel_stack_, (char*)kernel_stack_ + sizeof(kernel_stack_),
          working_dir_);

    initKernelStackCanary();
}

Thread::~Thread()
{
  debug(THREAD, "~Thread %s: freeing ThreadInfos\n", getName());
  // registers automatically destroyed via unique_ptr

  if(unlikely(holding_lock_list_ != nullptr))
  {
    debugAlways(THREAD, "~Thread: ERROR: Thread <%s [%zu] (%p)> is going to be destroyed, but still holds some locks!\n", getName(), getTID(), this);
    Lock::printHoldingList(this);
    assert(false && "~Thread: ERROR: Thread is going to be destroyed, but still holds some locks!\n");
  }
  debug(THREAD, "~Thread: done (%s)\n", name_.c_str());
}

// DO NOT use new / delete in this Method, as it is sometimes called from an Interrupt Handler with Interrupts disabled
void Thread::kill()
{
  debug(THREAD, "kill: Called by <%s (%p)>. Preparing Thread <%s (%p)> for destruction\n", currentThread->getName(), currentThread, getName(), this);

  setState(ToBeDestroyed); // vvv Code below this line may not be executed vvv

  if (currentThread == this)
  {
    ArchInterrupts::enableInterrupts();
    Scheduler::instance()->yield();
    assert(false && "Thread scheduled again after yield with state == ToBeDestroyed");
  }
}

void* Thread::getKernelStackStartPointer() const
{
  pointer stack = (pointer) kernel_stack_;
  stack += sizeof(kernel_stack_) - 2*sizeof(size_t);
  return (void*)stack;
}

void Thread::initKernelStackCanary()
{
    kernel_stack_[2047] = STACK_CANARY;
    kernel_stack_[0] = STACK_CANARY;
}

bool Thread::currentThreadIsStackCanaryOK()
{
    if (!CpuLocalStorage::ClsInitialized())
        return true;

    return !currentThread || currentThread->isStackCanaryOK();
}

// Hack to allow the function to be called from the klibc
// klibc is standalone and cannot include other SWEB code,
// but function can be declared with external linkage.
// We cannot use a class member function for that purpose
extern "C" bool currentThreadIsStackCanaryOK()
{
    return Thread::currentThreadIsStackCanaryOK();
}

bool Thread::isStackCanaryOK() const
{
  return kernel_stack_[0] == STACK_CANARY && kernel_stack_[2047] == STACK_CANARY;
}

Terminal* Thread::getTerminal() const
{
  return my_terminal_ ? my_terminal_ : main_console->getActiveTerminal();
}

void Thread::setTerminal(Terminal *my_term)
{
  my_terminal_ = my_term;
}

void Thread::printBacktrace()
{
  printBacktrace(currentThread != this);
}

FileSystemInfo* Thread::getWorkingDirInfo() const
{
  return working_dir_;
}

FileSystemInfo* getcwd()
{
  if (FileSystemInfo* info = currentThread->getWorkingDirInfo())
    return info;
  return default_working_dir;
}

void Thread::setWorkingDirInfo(FileSystemInfo* working_dir)
{
  working_dir_ = working_dir;
}

extern const Stabs2DebugInfo* kernel_debug_info;

void Thread::printBacktrace(bool use_stored_registers)
{
  if(!kernel_debug_info)
  {
    debug(BACKTRACE, "Kernel debug info not set up, backtrace won't look nice!\n");
  }

  static_assert(BACKTRACE_MAX_FRAMES < 128);
  pointer call_stack[BACKTRACE_MAX_FRAMES];
  size_t count = 0;

  if(kernel_debug_info)
  {
    count = backtrace(call_stack, BACKTRACE_MAX_FRAMES, this, use_stored_registers);

    debug(BACKTRACE, "=== Begin of backtrace for %sthread <%s>[%zu] (%p) ===\n", user_registers_ ? "user" : "kernel", getName(), getTID(), this);
    debug(BACKTRACE, " ----- Kernel -----------------------\n");
    for(size_t i = 0; i < count; ++i)
    {
        debug(BACKTRACE, " ");
        kernel_debug_info->printCallInformation(call_stack[i]);
    }
  }
  if(user_registers_)
  {
      const Stabs2DebugInfo* deb = loader_->getDebugInfos();
      count = backtrace_user(call_stack, BACKTRACE_MAX_FRAMES, this, false);
      debug(BACKTRACE, " ----- Userspace --------------------\n");
      if (!deb)
          debug(BACKTRACE,
                "Userspace debug info not set up, backtrace won't look nice!\n");
      else
      {
          for (size_t i = 0; i < count; ++i)
          {
              debug(BACKTRACE, " ");
              deb->printCallInformation(call_stack[i]);
          }
    }
  }
  debug(BACKTRACE, "=== End of backtrace for %sthread <%s>[%zu] (%p) ===\n", user_registers_ ? "user" : "kernel", getName(), getTID(), this);
}

bool Thread::schedulable()
{
  return (getState() == Running);
}

bool Thread::canRunOnCpu(size_t cpu_id) const
{
    return (pinned_to_cpu == (size_t)-1) || (pinned_to_cpu == cpu_id);
}

bool Thread::isCurrentlyScheduled() const
{
  return currently_scheduled_on_cpu_ != (size_t)-1;
}

bool Thread::isCurrentlyScheduledOnCpu(size_t cpu_id) const
{
    return currently_scheduled_on_cpu_ == cpu_id;
}

void Thread::setSchedulingStartTimestamp(uint64 timestamp)
{
    sched_start = timestamp;
}

uint64 Thread::schedulingStartTimestamp() const
{
    return sched_start;
}

const char *Thread::getName() const
{
  if(!this)
  {
    return "(nil)";
  }

  return name_.c_str();
}

size_t Thread::getTID() const
{
  return tid_;
}

Thread::ThreadState Thread::getState() const
{
  assert(this);
  return state_;
}

void Thread::setState(ThreadState new_state)
{
  assert(this);
  assert(!((state_ == ToBeDestroyed) && (new_state != ToBeDestroyed)) && "Tried to change thread state when thread was already set to be destroyed");
  assert(!((new_state == Sleeping) && (currentThread != this)) && "Setting other threads to sleep is not thread-safe");

  state_ = new_state;
}
