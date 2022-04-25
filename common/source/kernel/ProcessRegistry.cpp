#include <mm/KernelMemoryManager.h>
#include "ProcessRegistry.h"
#include "Scheduler.h"
#include "UserProcess.h"
#include "kprintf.h"
#include "VfsSyscall.h"
#include "ArchMulticore.h"
#include "VirtualFileSystem.h"

alignas(ProcessRegistry) unsigned char process_registry[sizeof(ProcessRegistry)];

ProcessRegistry* ProcessRegistry::instance_ = nullptr;

ProcessRegistry::ProcessRegistry(FileSystemInfo* root_fs_info) :
    default_working_dir_(root_fs_info),
    progs_running_(0),
    counter_lock_("ProcessRegistry::counter_lock_"),
    all_processes_killed_(&counter_lock_, "ProcessRegistry::all_processes_killed_")
{
  assert(default_working_dir_);
}

ProcessRegistry::~ProcessRegistry()
{
}

ProcessRegistry* ProcessRegistry::instance()
{
  assert(instance_ && "ProcessRegistry not yet initialized");
  return instance_;
}

void ProcessRegistry::init(FileSystemInfo *root_fs_info)
{
    assert(!instance_ && "ProcessRegistry already initialized");
    instance_ = new (&process_registry) ProcessRegistry(root_fs_info);
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

void ProcessRegistry::waitAllKilled()
{
    MutexLock l(counter_lock_);
    while (progs_running_)
        all_processes_killed_.wait();
}

void ProcessRegistry::createProcess(const char* path)
{
  Thread* process = new UserProcess(path, new FileSystemInfo(*default_working_dir_));
  debug(PROCESS_REG, "created userprocess %s\n", path);
  Scheduler::instance()->addNewThread(process);
  debug(PROCESS_REG, "added thread %s\n", path);
}
