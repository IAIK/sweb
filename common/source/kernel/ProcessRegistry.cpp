/**
 * @file ProcessRegistry.cpp
 */

#include "ProcessRegistry.h"
#include "Scheduler.h"
#include "UserProcess.h"
#include "kprintf.h"
#include "VfsSyscall.h"

extern VfsSyscall vfs_syscall;

ProcessRegistry* ProcessRegistry::instance_ = 0;

ProcessRegistry::ProcessRegistry(FileSystemInfo *root_fs_info, char const *progs[]) :
    Thread(root_fs_info, "ProcessRegistry"), progs_(progs), progs_running_(0),
    counter_lock_("ProcessRegistry::counter_lock_"),
    all_processes_killed_(&counter_lock_, "ProcessRegistry::all_processes_killed_")
{
  instance_ = this; // instance_ is static! -> Singleton-like behaviour
}

ProcessRegistry::~ProcessRegistry()
{
}

ProcessRegistry* ProcessRegistry::instance()
{
  return instance_;
}

void ProcessRegistry::Run()
{
  if (!progs_ || !progs_[0])
    return;

  debug(MOUNTMINIX, "mounting userprog-partition \n");

  vfs_syscall.mkdir("/usr", 0);
  debug(MOUNTMINIX, "mkdir /usr\n");
  vfs_syscall.mount("idea1", "/usr", "minixfs", 0);
  debug(MOUNTMINIX, "mount idea1\n");

  for (uint32 i = 0; progs_[i]; i++)
  {
    createProcess(progs_[i]);
  }

  counter_lock_.acquire();

  while (progs_running_)
    all_processes_killed_.wait();

  counter_lock_.release();

  debug(MOUNTMINIX, "unmounting userprog-partition because all processes terminated \n");

  vfs_syscall.umount("/user_progs", 0);

  kill();
}

void ProcessRegistry::processExit()
{
  counter_lock_.acquire();

  if (--progs_running_ == 0)
    all_processes_killed_.signal();

  counter_lock_.release();
}

void ProcessRegistry::processStart()
{
  counter_lock_.acquire();
  ++progs_running_;
  counter_lock_.release();
}

size_t ProcessRegistry::processCount()
{
  MutexLock lock(counter_lock_);
  return progs_running_;
}

void ProcessRegistry::createProcess(const char* path)
{
  debug(MOUNTMINIX, "create process %s\n", path);
  Thread* process = new UserProcess(path, new FileSystemInfo(*working_dir_), this);
  debug(MOUNTMINIX, "created userprocess %s\n", path);
  Scheduler::instance()->addNewThread(process);
  debug(MOUNTMINIX, "added thread %s\n", path);
}
