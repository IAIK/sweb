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

static void ThreadStartHack()
{
  currentThread->setTerminal ( main_console->getActiveTerminal() );
  currentThread->Run();
  currentThread->kill();
  debug ( THREAD,"ThreadStartHack: Panic, thread couldn't be killed\n" );
  for ( ;; );
}

Thread::Thread()
{
  debug ( THREAD,"Thread ctor, this is %x &s, stack is %x, sizeof stack is %x\r\n", this,stack_, sizeof ( stack_ ) );

  ArchThreads::createThreadInfosKernelThread ( kernel_arch_thread_info_, ( pointer ) &ThreadStartHack,getStackStartPointer() );
  user_arch_thread_info_=0;
  switch_to_userspace_ = 0;
  state_=Running;
  loader_ = 0;
  name_ = 0;
  my_terminal_ = 0;
  pid_ = 0;
  fs_info_ = new FileSystemInfo();
}

Thread::~Thread()
{
  if ( loader_ )
  {
    debug ( THREAD,"~Thread: cleaning up UserspaceAddressSpace (freeing Pages)\n" );
    loader_->cleanupUserspaceAddressSpace();
    delete loader_;
  }
  debug ( THREAD,"~Thread: freeing ThreadInfos\n" );
  ArchThreads::cleanupThreadInfos ( user_arch_thread_info_ ); //yes that's safe
  ArchThreads::cleanupThreadInfos ( kernel_arch_thread_info_ );
  debug ( THREAD,"~Thread: done\n" );
}

//if the Thread we want to kill, is the currentThread, we better not return
// DO Not use new / delete in this Method, as it sometimes called from an Interrupt Handler with Interrupts disabled
void Thread::kill()
{
  switch_to_userspace_ = false;
  state_=ToBeDestroyed;
  debug ( THREAD,"kill: Preparing currentThread (%x %s) for destruction\n",currentThread,currentThread->getName() );
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



