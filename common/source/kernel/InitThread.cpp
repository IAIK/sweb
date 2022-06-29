#include "InitThread.h"
#include "VfsSyscall.h"
#include <mm/KernelMemoryManager.h>
#include "ProcessRegistry.h"
#include "VirtualFileSystem.h"
#include "Scheduler.h"
#include "debug.h"

InitThread::InitThread(FileSystemInfo *root_fs_info, char const *progs[]) :
    Thread(root_fs_info, "InitThread", Thread::KERNEL_THREAD),
    progs_(progs)
{
}

InitThread::~InitThread()
{
}

void InitThread::Run()
{
  debug(INITTHREAD, "InitThread starting\n");

  assert(ArchInterrupts::testIFSet());

  if (!progs_ || !progs_[0])
    return;

  debug(INITTHREAD, "mounting userprog-partition \n");

  debug(INITTHREAD, "mkdir /usr\n");
  assert( !VfsSyscall::mkdir("/usr", 0) );

  // Mount user partition (initrd if it exists, else partition 1 of IDE drive A)
  bool usr_mounted = false;
  if (VfsSyscall::mount("initrd", "/usr", "minixfs", 0) == 0)
  {
      debug(INITTHREAD, "initrd mounted at /usr\n");
      usr_mounted = true;
  }
  else if (VfsSyscall::mount("idea1", "/usr", "minixfs", 0) == 0)
  {
      debug(INITTHREAD, "idea1 mounted at /usr\n");
      usr_mounted = true;
  }

  assert(usr_mounted && "Unable to mount userspace partition");

  debug(INITTHREAD, "mkdir /dev\n");
  assert( !VfsSyscall::mkdir("/dev", 0) );
  debug(INITTHREAD, "mount devicefs\n");
  assert( !VfsSyscall::mount(NULL, "/dev", "devicefs", 0) );

  KernelMemoryManager::instance()->startTracing();

  debug(INITTHREAD, "Starting user processes\n");

  for (uint32 i = 0; progs_[i]; i++)
  {
    debug(INITTHREAD, "Starting %s\n", progs_[i]);
    kprintf("Starting %s\n", progs_[i]);
    ProcessRegistry::instance()->createProcess(progs_[i]);
  }

  ProcessRegistry::instance()->waitAllKilled();

  kprintf("All processes terminated\n");
  debug(INITTHREAD, "unmounting userprog-partition because all processes terminated \n");

  VfsSyscall::umount("/usr", 0);
  VfsSyscall::umount("/dev", 0);
  vfs.rootUmount();

  Scheduler::instance()->printStackTraces();

  Scheduler::instance()->printThreadList();

  kill();
}
