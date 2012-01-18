/**
 * @file MountMinix.cpp
 */

#include "MountMinix.h"
#include "Scheduler.h"
#include "UserProcess.h"
#include "fs_global.h"
#include "kprintf.h"

MountMinixAndStartUserProgramsThread* MountMinixAndStartUserProgramsThread::instance_ = 0;

MountMinixAndStartUserProgramsThread::MountMinixAndStartUserProgramsThread
                          ( FileSystemInfo *root_fs_info, char const *progs[] ) :
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

  debug(MOUNTMINIX, "mounting userprog-partition \n");

  vfs_syscall.mkdir ( "/user_progs", 0 );
  debug(MOUNTMINIX, "mkdir /user_progs\n");
  vfs_syscall.mount ( "idea1", "/user_progs", "minixfs", 0 );
  debug(MOUNTMINIX, "mount idea1\n");

  for(uint32 i=0; progs_[i]; i++)
  {
    createProcess(progs_[i]);
  }

  counter_lock_.acquire();

  while(progs_running_)
    all_processes_killed_.wait();

  counter_lock_.release();

  debug(MOUNTMINIX, "unmounting userprog-partition because all processes terminated \n");

  vfs_syscall.umount ("/user_progs", 0 );
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
  Thread* process = new UserProcess(path, new FileSystemInfo(*fs_info_), this);
  debug(MOUNTMINIX, "created userprocess %s\n", path);
  Scheduler::instance()->addNewThread(process);
  debug(MOUNTMINIX, "added thread %s\n", path);
  return process;
}
