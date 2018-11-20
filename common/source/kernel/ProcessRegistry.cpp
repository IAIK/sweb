#include <mm/KernelMemoryManager.h>
#include "ProcessRegistry.h"
#include "Scheduler.h"
#include "UserProcess.h"
#include "kprintf.h"
#include "VfsSyscall.h"
#include "ArchMulticore.h"


ProcessRegistry* ProcessRegistry::instance_ = 0;

ProcessRegistry::ProcessRegistry(FileSystemInfo *root_fs_info, char const *progs[]) :
    Thread(root_fs_info, "ProcessRegistry", Thread::KERNEL_THREAD), progs_(progs), progs_running_(0),
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

  debug(PROCESS_REG, "mounting userprog-partition \n");

  debug(PROCESS_REG, "mkdir /usr\n");
  VfsSyscall::mkdir("/usr", 0);

  //debug(PROCESS_REG, "mount idea1\n");
  //VfsSyscall::mount("idea1", "/usr", "minixfs", 0);

  //debug(PROCESS_REG, "mkdir /initrd\n");
  //VfsSyscall::mkdir("/initrd", 0);

  debug(PROCESS_REG, "mount initrd to /usr\n");
  kprintf("Mount initrd\n");
  VfsSyscall::mount("initrd", "/usr", "minixfs", 0);

  KernelMemoryManager::instance()->startTracing();

  debug(PROCESS_REG, "Starting user processes\n");

  for(auto cls : ArchMulticore::cpu_list_)
  {
          assert(cls->getCpuID() <= 4);
          debug(MAIN, "Starting helloworld on CPU %zu\n", cls->getCpuID());
          createProcess("/usr/helloworld.sweb", cls->getCpuID());
  }

  for (uint32 i = 0; progs_[i]; i++)
  {
    debug(PROCESS_REG, "Starting %s\n", progs_[i]);
    kprintf("Starting %s\n", progs_[i]);
    createProcess(progs_[i]);
  }

  counter_lock_.acquire();

  while (progs_running_)
    all_processes_killed_.wait();

  counter_lock_.release();

  kprintf("All processes terminated\n");
  debug(PROCESS_REG, "unmounting userprog-partition because all processes terminated \n");

  VfsSyscall::umount("/usr", 0);

  Scheduler::instance()->printStackTraces();

  Scheduler::instance()->printThreadList();

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

void ProcessRegistry::createProcess(const char* path, size_t cpu)
{
  debug(PROCESS_REG, "create process %s on cpu %zu\n", path, cpu);
  Thread* process = new UserProcess(path, new FileSystemInfo(*working_dir_));
  debug(PROCESS_REG, "created userprocess %s\n", path);
  /*
  if(cpu == (size_t)-1)
  {
          cpu_scheduler.addNewThread(process);
  }
  else
  {
          bool cpu_found = false;

          for(auto cls : ArchMulticore::cpu_list_)
          {
                  if(cls->getCpuID() == cpu)
                  {
                          cpu_found = true;
                          cls->getScheduler()->addNewThread(process);
                          break;
                  }
          }
          assert(cpu_found);
  }
  */
  Scheduler::instance()->addNewThread(process);
  debug(PROCESS_REG, "added thread %s\n", path);
}
