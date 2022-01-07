#include <mm/KernelMemoryManager.h>
#include "ProcessRegistry.h"
#include "Scheduler.h"
#include "UserProcess.h"
#include "kprintf.h"
#include "VfsSyscall.h"
#include "ArchMulticore.h"


ProcessRegistry* ProcessRegistry::instance_ = nullptr;

ProcessRegistry::ProcessRegistry(FileSystemInfo *root_fs_info, char const *progs[]) :
    Thread(root_fs_info, "ProcessRegistry", Thread::KERNEL_THREAD), progs_(progs), progs_running_(0),
    counter_lock_("ProcessRegistry::counter_lock_"),
    all_processes_killed_(&counter_lock_, "ProcessRegistry::all_processes_killed_")
{
  instance_ = this; // instance_ is static! -> Singleton-like behaviour
}

ProcessRegistry::~ProcessRegistry()
{
        // debug
        assert(false && "Process registry shouldn't die (while testing)");
}

ProcessRegistry* ProcessRegistry::instance()
{
  return instance_;
}

void ProcessRegistry::Run()
{
  if (!progs_ || !progs_[0])
    return;

  debug(PROCESS_REG, "mounting userprog-partition \n");

  debug(PROCESS_REG, "mkdir /usr\n");
  VfsSyscall::mkdir("/usr", 0);

  // Mount user partition (initrd if it exists, else partition 1 of IDE drive A)
  bool usr_mounted = false;
  if (VfsSyscall::mount("initrd", "/usr", "minixfs", 0) == 0)
  {
      debug(PROCESS_REG, "initrd mounted at /usr\n");
      usr_mounted = true;
  }
  else if (VfsSyscall::mount("idea1", "/usr", "minixfs", 0) == 0)
  {
      debug(PROCESS_REG, "idea1 mounted at /usr\n");
      usr_mounted = true;
  }

  assert(usr_mounted && "Unable to mount userspace partition");

  KernelMemoryManager::instance()->startTracing();

  debug(PROCESS_REG, "Starting user processes\n");

  for (uint32 i = 0; progs_[i]; i++)
  {
    debug(PROCESS_REG, "Starting %s\n", progs_[i]);
    kprintf("Starting %s\n", progs_[i]);
    createProcess(progs_[i]);
  }

  {
    MutexLock l(counter_lock_);
    while (progs_running_)
      all_processes_killed_.wait();
  }

  kprintf("All processes terminated\n");
  debug(PROCESS_REG, "unmounting userprog-partition because all processes terminated \n");

  VfsSyscall::umount("/usr", 0);

  Scheduler::instance()->printStackTraces();

  Scheduler::instance()->printThreadList();

  kill();
}

void ProcessRegistry::processExit()
{
  MutexLock l(counter_lock_);

  if (--progs_running_ == 0)
    all_processes_killed_.broadcast();
}

void ProcessRegistry::processStart()
{
  MutexLock l(counter_lock_);
  ++progs_running_;
}

size_t ProcessRegistry::processCount()
{
  // Note that the returned count value is out of date as soon
  // as the lock is released
  MutexLock lock(counter_lock_);
  return progs_running_;
}

void ProcessRegistry::createProcess(const char* path)
{
  Thread* process = new UserProcess(path, new FileSystemInfo(*working_dir_));
  debug(PROCESS_REG, "created userprocess %s\n", path);
  Scheduler::instance()->addNewThread(process);
  debug(PROCESS_REG, "added thread %s\n", path);
}
