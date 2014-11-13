/**
 * @file Thread.cpp
 */

#include "kernel/Thread.h"
#include "ArchCommon.h"
#include "console/kprintf.h"
#include "ArchThreads.h"
#include "ArchInterrupts.h"
#include "Scheduler.h"
#include "Loader.h"
#include "console/Console.h"
#include "console/Terminal.h"
#include "backtrace.h"
#include "mm/KernelMemoryManager.h"
#include "Stabs2DebugInfo.h"

#define MAX_STACK_FRAMES 20

const char* Thread::threadStatePrintable[3] = {"Running", "Sleeping", "ToBeDestroyed"};

static void ThreadStartHack()
{
  currentThread->setTerminal ( main_console->getActiveTerminal() );
  currentThread->Run();
  currentThread->kill();
  debug ( THREAD,"ThreadStartHack: Panic, thread couldn't be killed\n" );
  for ( ;; ) ;
}

Thread::Thread(const char *name) :
  kernel_arch_thread_info_(0),
  user_arch_thread_info_(0),
  switch_to_userspace_(0),
  loader_(0),
  state_(Running),
  sleeping_on_mutex_(0),
  tid_(0),
  my_terminal_(0),
  working_dir_(0),
  name_(name)
{
  debug ( THREAD,"Thread ctor, this is %x; stack is %x\n", this, stack_ );
  debug ( THREAD,"sizeof stack is %x; my name: %s\n", sizeof ( stack_ ), name_ ); 
  ArchThreads::createThreadInfosKernelThread ( kernel_arch_thread_info_, ( pointer ) &ThreadStartHack,getStackStartPointer() );
}

Thread::Thread ( FsWorkingDirectory *working_dir, const char *name ) :
  kernel_arch_thread_info_(0),
  user_arch_thread_info_(0),
  switch_to_userspace_(0),
  loader_(0),
  state_(Running),
  sleeping_on_mutex_(0),
  tid_(0),
  my_terminal_(0),
  working_dir_(working_dir),
  name_(name)
{
  debug ( THREAD,"Thread ctor, this is %x, stack is %x\n", this, stack_);
  debug ( THREAD,"sizeof stack is %x; my name: %s\n", sizeof ( stack_ ), name_ ); 
  debug ( THREAD,"Thread ctor, fs_info ptr: %x\n", working_dir_ );
  ArchThreads::createThreadInfosKernelThread ( kernel_arch_thread_info_, ( pointer ) &ThreadStartHack,getStackStartPointer() );
}

Thread::~Thread()
{
  if ( loader_ )
  {
    debug ( THREAD,"~Thread: cleaning up UserspaceAddressSpace (freeing Pages)\n" );
    delete loader_;
    loader_ = 0;
  }
  debug ( THREAD,"~Thread: freeing ThreadInfos\n" );
  ArchThreads::cleanupThreadInfos ( user_arch_thread_info_ ); //yes that's safe
  user_arch_thread_info_ = 0;
  ArchThreads::cleanupThreadInfos ( kernel_arch_thread_info_ );
  kernel_arch_thread_info_ = 0;
  if ( working_dir_ )
  {
    debug ( THREAD,"~Thread deleting fs info\n" );
    delete working_dir_;
    working_dir_ = 0;
  }
  debug ( THREAD,"~Thread: done (%s)\n", name_ );
  assert(KernelMemoryManager::instance()->KMMLockHeldBy() != this);
}

//if the Thread we want to kill, is the currentThread, we better not return
// DO Not use new / delete in this Method, as it sometimes called from an Interrupt Handler with Interrupts disabled
void Thread::kill()
{
  debug ( THREAD,"kill: Called by <%s (%x)>. Preparing Thread <%s (%x)> for destruction\n", currentThread->getName(),
      currentThread, getName(), this);

  switch_to_userspace_ = false;
  state_=ToBeDestroyed;

  if ( currentThread == this )
  {
    ArchInterrupts::enableInterrupts();
    Scheduler::instance()->yield();
  }
}

pointer Thread::getStackStartPointer()
{
  pointer stack = ( pointer ) stack_;
  stack += sizeof ( stack_ ) - sizeof ( uint32 );
  return stack;
}

Terminal *Thread::getTerminal()
{
  if ( my_terminal_ )
    return my_terminal_;
  else
    return ( main_console->getActiveTerminal() );
}

void Thread::setTerminal ( Terminal *my_term )
{
  my_terminal_=my_term;
}

void Thread::printBacktrace()
{
  printBacktrace(currentThread != this);
}

FsWorkingDirectory* Thread::getWorkingDirInfo(void)
{
  return working_dir_;
}

void Thread::setWorkingDirInfo(FsWorkingDirectory* working_dir)
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

  pointer CallStack[MAX_STACK_FRAMES];
  int Count = backtrace(CallStack, MAX_STACK_FRAMES,
      this, use_stored_registers);

  debug(BACKTRACE, "=== Begin of backtrace for kernel thread <%s> ===\n", getName());
  debug(BACKTRACE, "   found <%d> stack %s:\n", Count, Count != 1 ? "frames" : "frame");
  debug(BACKTRACE, "\n");

  for (int i = 0; i < Count; ++i)
  {
    char FunctionName[255];
    pointer StartAddr = 0;
    if (kernel_debug_info)
      StartAddr = kernel_debug_info->getFunctionName(CallStack[i], FunctionName);

    if (StartAddr)
    {
      ssize_t line = kernel_debug_info->getFunctionLine(StartAddr,CallStack[i] - StartAddr);
      if (line > 0)
        debug(BACKTRACE, "   (%d): %010x (%s:%u)\n", i, CallStack[i], FunctionName, line);
      else
        debug(BACKTRACE, "   (%d): %010x (%s+%x)\n", i, CallStack[i], FunctionName, CallStack[i] - StartAddr);
    }
    else
      debug(BACKTRACE, "   (%d): %010x (<UNKNOWN FUNCTION>)\n", i, CallStack[i]);
  }

  debug(BACKTRACE, "=== End of backtrace for thread <%s> ===\n", getName());
}

void Thread::printUserBacktrace()
{
  if (!user_arch_thread_info_)
  {
    debug(US_BACKTRACE, "=== Can not do userspace stacktracing of thread <%s> since it has no userspace! ===\n", getName());
  }

  pointer CallStack[MAX_STACK_FRAMES];
  int Count = backtrace_user(CallStack, MAX_STACK_FRAMES, this, 0);

  debug(US_BACKTRACE, "=== Begin of backtrace for user thread <%s> ===\n", getName());
  debug(US_BACKTRACE, "   found <%d> stack %s:\n", Count, Count != 1 ? "frames" : "frame");
  debug(US_BACKTRACE, "\n");

  Stabs2DebugInfo const *deb = 0;
  if (loader_)
    deb = loader_->getDebugInfos();

  for (int i = 0; i < Count; ++i)
  {
    char FunctionName[255];
    pointer StartAddr = 0;
    if (deb)
      StartAddr = deb->getFunctionName(CallStack[i], FunctionName);

    if (StartAddr)
    {
      ssize_t line = deb->getFunctionLine(StartAddr,CallStack[i] - StartAddr);
      if (line > 0)
        debug(US_BACKTRACE, "   (%d): %010x (%s:%u)\n", i, CallStack[i], FunctionName, line);
      else
        debug(US_BACKTRACE, "   (%d): %010x (%s+%x)\n", i, CallStack[i], FunctionName, CallStack[i] - StartAddr);
    }
    else
      debug(US_BACKTRACE, "   (%d): %010x (<UNKNOWN FUNCTION>)\n", i, CallStack[i]);
  }

  debug(US_BACKTRACE, "=== End of backtrace for thread <%s> ===\n", getName());
}


