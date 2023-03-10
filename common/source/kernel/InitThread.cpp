#include "InitThread.h"
#include "VfsSyscall.h"
#include <mm/KernelMemoryManager.h>
#include "ProcessRegistry.h"
#include "VirtualFileSystem.h"
#include "DeviceFSSuperblock.h"
#include "DeviceBus.h"
#include "BootloaderModules.h"
#include "PageManager.h"
#include "Scheduler.h"
#include "debug.h"
#include "kprintf.h"

InitThread* InitThread::instance_ = nullptr;

InitThread::InitThread(FileSystemInfo* root_fs_info, const char* progs[]) :
    Thread(root_fs_info, "InitThread", Thread::KERNEL_THREAD),
    progs_(progs)
{
}

InitThread::~InitThread()
{
}

void InitThread::init(FileSystemInfo* root_fs_info, const char* progs[])
{
    assert(!instance_ && "InitThread already initialized");
    // Need to allocate this on the heap -> gets deleted by cleanup thread
    instance_ = new InitThread(root_fs_info, progs);
}

InitThread* InitThread::instance()
{
    assert(instance_ && "InitThread not yet initialized");
    return instance_;
}


void InitThread::Run()
{
    debug(INITTHREAD, "InitThread starting\n");

    assert(ArchInterrupts::testIFSet());

    debug(INITTHREAD, "Checking for initrd\n");
    BootloaderModules::loadInitrdIfExists();

    debug(INITTHREAD, "Block Device creation\n");
    ArchCommon::initBlockDeviceDrivers();

    debug(INITTHREAD, "Registered devices:\n");
    deviceTreeRoot().printSubDevices();

    debug(INITTHREAD, "Add devices to devicefs\n");
    DeviceFSSuperBlock::getInstance()->addBlockDeviceInodes();
    DeviceFSSuperBlock::getInstance()->addDeviceInodes(deviceTreeRoot());

    if (!progs_ || !progs_[0])
        return;

    size_t free_pages_pre = PageManager::instance().getNumFreePages();

    debug(INITTHREAD, "mounting userprog-partition\n");

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

    if (!usr_mounted)
        kprintf("ERROR: Unable to mount userspace partition\n");
    assert(usr_mounted && "Unable to mount userspace partition");

    debug(INITTHREAD, "mkdir /dev\n");
    assert( !VfsSyscall::mkdir("/dev", 0) );
    debug(INITTHREAD, "mount devicefs\n");
    assert( !VfsSyscall::mount(nullptr, "/dev", "devicefs", 0) );

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



    debug(INITTHREAD, "unmounting userprog-partition because all processes terminated\n");

    VfsSyscall::umount("/usr", 0);
    VfsSyscall::umount("/dev", 0);

    size_t free_pages_post = PageManager::instance().getNumFreePages();
    if ((free_pages_pre != free_pages_post) && !DYNAMIC_KMM)
    {
            PageManager::instance().printUsageInfo();
            debugAlways(PM, "WARNING: You might be leaking physical memory pages somewhere\n");
            debugAlways(PM, "%zu/%zu free physical pages after unmounting detected, difference: %zd\n",
                        free_pages_post,
                        free_pages_pre,
                        free_pages_pre - free_pages_post);
    }

    vfs.rootUmount();

    Scheduler::instance()->printStackTraces();
    Scheduler::instance()->printThreadList();

    kill();
}
