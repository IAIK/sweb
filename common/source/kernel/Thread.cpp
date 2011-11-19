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

#define MAX_STACK_FRAMES 20

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
  pid_(0),
  my_terminal_(0),
  fs_info_(new FileSystemInfo()),
  name_(name)
{
  debug ( THREAD,"Thread ctor, this is %x; stack is %x\n", this, stack_ );
  debug ( THREAD,"sizeof stack is %x; my name: %s\n", sizeof ( stack_ ), name_ ); 
  ArchThreads::createThreadInfosKernelThread ( kernel_arch_thread_info_, ( pointer ) &ThreadStartHack,getStackStartPointer() );
}

Thread::Thread ( FileSystemInfo *fs_info, const char *name ) :
  kernel_arch_thread_info_(0),
  user_arch_thread_info_(0),
  switch_to_userspace_(0),
  loader_(0),
  state_(Running),
  pid_(0),
  my_terminal_(0),
  fs_info_(fs_info),
  name_(name)
{
  debug ( THREAD,"Thread ctor, this is %x, stack is %x\n", this, stack_);
  debug ( THREAD,"sizeof stack is %x; my name: %s\n", sizeof ( stack_ ), name_ ); 
  debug ( THREAD,"Thread ctor, fs_info ptr: %x\n", fs_info );
  ArchThreads::createThreadInfosKernelThread ( kernel_arch_thread_info_, ( pointer ) &ThreadStartHack,getStackStartPointer() );
}

Thread::~Thread()
{
  if ( loader_ )
  {
    debug ( THREAD,"~Thread: cleaning up UserspaceAddressSpace (freeing Pages)\n" );
    loader_->cleanupUserspaceAddressSpace();
    delete loader_;
    loader_ = 0;
  }
  debug ( THREAD,"~Thread: freeing ThreadInfos\n" );
  ArchThreads::cleanupThreadInfos ( user_arch_thread_info_ ); //yes that's safe
  user_arch_thread_info_ = 0;
  ArchThreads::cleanupThreadInfos ( kernel_arch_thread_info_ );
  kernel_arch_thread_info_ = 0;
  if ( fs_info_ )
  {
    debug ( THREAD,"~Thread deleting fs info\n" );
    delete fs_info_;
    fs_info_ = 0;
  }
  debug ( THREAD,"~Thread: done (%s)\n", name_ );
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

FileSystemInfo *Thread::getFSInfo()
{
  return fs_info_;
}

void Thread::setFSInfo ( FileSystemInfo *fs_info )
{
  fs_info_ = fs_info;
}

void Thread::printBacktrace(bool use_stored_registers)
{
  pointer CallStack[MAX_STACK_FRAMES];
  int Count = backtrace(CallStack, MAX_STACK_FRAMES,
      this, use_stored_registers);

  debug(BACKTRACE, "=== Begin of backtrace for thread <%s> ===\n", getName());
  debug(BACKTRACE, "   found <%d> stack %s:\n\n", Count, Count != 1 ? "frames" : "frame");

  for (int i = 0; i < Count; ++i)
  {
    char FunctionName[255];
    pointer StartAddr = get_function_name(CallStack[i], FunctionName);

    if (StartAddr)
      debug(BACKTRACE, "   (%d): %x (%s+%x)\n", i, CallStack[i], FunctionName, CallStack[i] - StartAddr);
    else
      debug(BACKTRACE, "   (%d): %x (<UNKNOWN FUNCTION>)\n", i, CallStack[i]);
  }

  debug(BACKTRACE, "=== End of backtrace for thread <%s> ===\n", getName());
}
