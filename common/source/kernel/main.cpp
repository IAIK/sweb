#include <ProcessRegistry.h>
#include "InitThread.h"
#include <types.h>
#include "ArchSerialInfo.h"
#include "BDManager.h"
#include "BDVirtualDevice.h"
#include "PageManager.h"
#include "KernelMemoryManager.h"
#include "ArchInterrupts.h"
#include "ArchThreads.h"
#include "kprintf.h"
#include "Thread.h"
#include "Scheduler.h"
#include "ArchCommon.h"
#include "ArchThreads.h"
#include "Mutex.h"
#include "debug_bochs.h"
#include "ArchMemory.h"
#include "Loader.h"
#include "assert.h"
#include "SerialManager.h"
#include "KeyboardManager.h"
#include "VfsSyscall.h"
#include "FileSystemInfo.h"
#include "Dentry.h"
#include "DeviceFSType.h"
#include "DeviceFSSuperblock.h"
#include "RamFSType.h"
#include "MinixFSType.h"
#include "VirtualFileSystem.h"
#include "VfsSyscall.h"
#include "FileDescriptor.h"
#include "TextConsole.h"
#include "FrameBufferConsole.h"
#include "Terminal.h"
#include "user_progs.h"
#include "RamDiskDriver.h"
#include "ArchMulticore.h"
#include "BlockDeviceInode.h"
#include "BootloaderModules.h"

extern void* kernel_end_address;

uint8 boot_stack[0x4000] __attribute__((aligned(0x4000)));
SystemState system_state;
FileSystemInfo* default_working_dir;

extern "C" void removeBootTimeIdentMapping();

BDVirtualDevice* createRamDiskFromModule(int module_num, const char* name)
{
    size_t ramdisk_size = ArchCommon::getModuleEndAddress(module_num) - ArchCommon::getModuleStartAddress(module_num);
    debug(MAIN, "Creating ram disk from module %s at [%zx, %zx), size: %zx\n", ArchCommon::getModuleName(module_num), ArchCommon::getModuleStartAddress(module_num), ArchCommon::getModuleEndAddress(module_num), ramdisk_size);
    return RamDiskDriver::createRamDisk((void*)ArchCommon::getModuleStartAddress(module_num), ramdisk_size, name);
}

void loadInitrd()
{
    // TODO: ArchCommon::getModuleEndAddress(i) -> getKernelEndAddress() crashes on arm rpi2

    for(size_t i = 0; i < ArchCommon::getNumModules(); ++i)
    {
        debug(MAIN, "Checking module %zu: %s\n", i, ArchCommon::getModuleName(i));

        if(strcmp(ArchCommon::getModuleName(i), "/boot/initrd") == 0)
        {
            debug(MAIN, "Initialize initrd\n");
            BDVirtualDevice* initrd_dev = createRamDiskFromModule(i, "initrd");
            initrd_dev->setPartitionType(0x81);
            BDManager::getInstance()->addVirtualDevice(initrd_dev);
            break;
        }
    }
}


extern "C" [[noreturn]] void startup()
{
  system_state = BOOTING;

  debug(MAIN, "Initializing kernel arch mem\n");
  ArchMemory::initKernelArchMem();

  debug(MAIN, "Initializing kernel memory management\n");
  PageManager::init();
  debug(MAIN, "PageManager and KernelMemoryManager created \n");

  BootloaderModules::mapModules();

  ArchCommon::postBootInit();

  debug(MAIN, "SMP init\n");
  SMP::initialize();

  debug(MAIN, "Interrupts init\n");
  ArchInterrupts::initialise();

  debug(MAIN, "Creating console\n");
  main_console = ArchCommon::createConsole(1);
  debug(MAIN, "Console created\n");

  Terminal *term_0 = main_console->getTerminal(0); // add more if you need more...
  term_0->initTerminalColors(CONSOLECOLOR::GREEN, CONSOLECOLOR::BLACK);
  kprintfd("Init debug printf\n");
  term_0->writeString("This is on term 0, you should see me now\n");
  debug(MAIN, "SetActiveTerminal 0\n");

  main_console->setActiveTerminal(0);

  kprintf("Kernel end address is %p\n", &kernel_end_address);

  debug(MAIN, "Scheduler init\n");
  Scheduler::instance();

  //needs to be done after scheduler and terminal, but prior to enableInterrupts
  kprintf_init();

  debug(MAIN, "Threads init\n");
  ArchThreads::initialise();

  debug(MAIN, "Multicore start CPUs\n");
  ArchMulticore::startOtherCPUs();


  debug(MAIN, "Removing Boot Time Ident Mapping...\n");
  removeBootTimeIdentMapping();

  debug(MAIN, "Setting scheduler tick frequency...\n");
  ArchInterrupts::setTimerFrequency(IRQ0_TIMER_FREQUENCY);

  debug(MAIN, "Init debug...\n");
  ArchCommon::initDebug();

  debug(MAIN, "Init VFS...\n");
  new (&VfsSyscall::vfs_lock) Mutex("vfs lock");
  vfs.initialize();
  debug(MAIN, "Mounting root file system\n");
  vfs.registerFileSystem(DeviceFSType::getInstance());
  vfs.registerFileSystem(new RamFSType());
  vfs.registerFileSystem(new MinixFSType());
  default_working_dir = vfs.rootMount("ramfs", 0);
  assert(default_working_dir);

  // initialise global and static objects
  new (&global_fd_list) FileDescriptorList();

  debug(MAIN, "Block Device creation\n");
  BDManager::getInstance()->doDeviceDetection();
  debug(MAIN, "Block Device done\n");
  debug(MAIN, "Checking for initrd\n");

  loadInitrd();

  debug(MAIN, "Add block devices to devicefs\n");
  auto devicefs_sb = DeviceFSSuperBlock::getInstance();

  for (BDVirtualDevice* bdvd : BDManager::getInstance()->device_list_)
  {
    debug(MAIN, "Detected Device: %s :: %d\n", bdvd->getName(), bdvd->getDeviceNumber());
    kprintf("Detected Device: %s :: %d\n", bdvd->getName(), bdvd->getDeviceNumber());
    auto bdInode = new BlockDeviceInode(bdvd);
    devicefs_sb->addDevice(bdInode, bdvd->getName());
  }


  debug(MAIN, "make a deep copy of FsWorkingDir\n");
  main_console->setWorkingDirInfo(new FileSystemInfo(*default_working_dir));
  debug(MAIN, "main_console->setWorkingDirInfo done\n");

  debug(MAIN, "default_working_dir root name: %s\t pwd name: %s\n",
        default_working_dir->getRoot().dentry_->getName(),
        default_working_dir->getPwd().dentry_->getName());
  if (main_console->getWorkingDirInfo())
  {
    delete main_console->getWorkingDirInfo();
  }
  main_console->setWorkingDirInfo(default_working_dir);

  ProcessRegistry::init(default_working_dir);

  debug(MAIN, "Timer enable\n");
  ArchInterrupts::enableTimer();

  KeyboardManager::instance();
  ArchInterrupts::enableKBD();

  debug(MAIN, "Adding Kernel threads\n");
  Scheduler::instance()->addNewThread(main_console);
  auto init_thread = new InitThread(new FileSystemInfo(*default_working_dir), user_progs /*see user_progs.h*/);
  Scheduler::instance()->addNewThread(init_thread);
  Scheduler::instance()->printThreadList();

  debug(MAIN, "%zu CPU(s) running\n", SMP::cpu_list_.size());
  kprintf("%zu CPU(s) running\n", SMP::cpu_list_.size());
  for(auto cls : SMP::cpu_list_)
  {
      debug(MAIN, "CPU %zu\n", cls->getCpuID());
      kprintf("CPU %zu\n", cls->getCpuID());
  }

  // Ensure we already have a currentThread when interrupts are enabled
  debug(MAIN, "Starting threads and enabling interrupts...\n");
  system_state = RUNNING;

  ArchThreads::startThreads(init_thread);

  //not reached
  assert(false && "Reached end of startup()");
}
