/**
 * @file MountMinix.cpp
 */

#include "MountMinix.h"
#include "Scheduler.h"
#include "UserProcess.h"
#include "kprintf.h"
#include "fs/VfsSyscall.h"

MountMinixAndStartUserProgramsThread* MountMinixAndStartUserProgramsThread::instance_ = 0;

MountMinixAndStartUserProgramsThread::MountMinixAndStartUserProgramsThread
                          ( FsWorkingDirectory *root_fs_info, char const ** progs ) :
  Thread ( root_fs_info, "MountMinixAndStartUserProgramsThread" ),
  progs_(progs),
  progs_running_(0),
  counter_lock_("MountMinixAndStartUserProgramsThread::counter_lock_"),
  all_processes_killed_(&counter_lock_)
{
  instance_ = this; // instance_ is static! attention if you make changes in number of MountMinixThreads or similar
}

MountMinixAndStartUserProgramsThread* MountMinixAndStartUserProgramsThread::instance()
{
  return instance_;
}

void MountMinixAndStartUserProgramsThread::Run()
{
  if(!progs_ || !progs_[0])
    return;

  for(uint32 i=0; progs_[i]; i++)
  {
    createProcess(progs_[i]);
  }

  counter_lock_.acquire();

  while(progs_running_)
    all_processes_killed_.wait();

  counter_lock_.release();

  debug(MOUNTMINIX, "unmounting userprog-partition because all processes terminated \n");

  delete working_dir_;
  working_dir_ = NULL;

  VfsSyscall::instance()->unmountRoot();
  debug(MOUNTMINIX, "VfsSyscall was successfully shut-down! \n");

  kill();
}

void MountMinixAndStartUserProgramsThread::processExit()
{
  counter_lock_.acquire();

  if(--progs_running_ == 0)
    all_processes_killed_.signal();

  counter_lock_.release();
}

void MountMinixAndStartUserProgramsThread::processStart()
{
  counter_lock_.acquire();
  ++progs_running_;
  counter_lock_.release();
}

Thread* MountMinixAndStartUserProgramsThread::createProcess(const char* path)
{
  debug(MOUNTMINIX, "create process %s\n", path);
  Thread* process = new UserProcess(path, new FsWorkingDirectory(*working_dir_), this);
  debug(MOUNTMINIX, "created userprocess %s\n", path);
  Scheduler::instance()->addNewThread(process);
  debug(MOUNTMINIX, "added thread %s\n", path);
  return process;
}
