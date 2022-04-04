#include <ProcessRegistry.h>
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
#include "RamFSType.h"
#include "MinixFSType.h"
#include "VirtualFileSystem.h"
#include "VfsSyscall.h"
#include "FileDescriptor.h"
#include "TextConsole.h"
#include "FrameBufferConsole.h"
#include "Terminal.h"
#include "outerrstream.h"
#include "user_progs.h"
#include "RamDiskDriver.h"
#include "ArchMulticore.h"

extern void* kernel_end_address;

uint8 boot_stack[0x4000] __attribute__((aligned(0x4000)));
SystemState system_state;
FileSystemInfo* default_working_dir;

extern "C" void initialiseBootTimePaging();
extern "C" void removeBootTimeIdentMapping();

extern "C" [[noreturn]] void startup()
{
  system_state = BOOTING;

  PageManager::init();
  debug(MAIN, "PageManager and KernelMemoryManager created \n");

  // TODO: move this function out of the page manager
  PageManager::instance()->mapModules();

  ArchCommon::postBootInit();

  debug(MAIN, "Multicore init\n");
  ArchMulticore::initialize();

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

  ArchInterrupts::setTimerFrequency(IRQ0_TIMER_FREQUENCY);

  ArchCommon::initDebug();

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

  for(size_t i = 0; i < ArchCommon::getNumModules(); ++i)
  {
          debug(MAIN, "Module %s [%zx, %zx)\n", ArchCommon::getModuleName(i), ArchCommon::getModuleStartAddress(i), ArchCommon::getModuleEndAddress(i));
          if(strcmp(ArchCommon::getModuleName(i), "/boot/initrd") == 0)
          {
              debug(MAIN, "Initialize initrd\n");
                  size_t initrd_size = ArchCommon::getModuleEndAddress(i) - ArchCommon::getModuleStartAddress(i);
                  debug(MAIN, "Found initrd module %s at [%zx, %zx), size: %zx\n", ArchCommon::getModuleName(i), ArchCommon::getModuleStartAddress(i), ArchCommon::getModuleEndAddress(i), initrd_size);
                  BDVirtualDevice* initrd_dev = new BDVirtualDevice(new RamDiskDriver((void*)ArchCommon::getModuleStartAddress(i), initrd_size), 0, initrd_size, 1, "initrd", true);
                  initrd_dev->setPartitionType(0x81);
                  BDManager::getInstance()->addVirtualDevice(initrd_dev);
                  break;
          }
  }

  for (BDVirtualDevice* bdvd : BDManager::getInstance()->device_list_)
  {
    debug(MAIN, "Detected Device: %s :: %d\n", bdvd->getName(), bdvd->getDeviceNumber());
    kprintf("Detected Device: %s :: %d\n", bdvd->getName(), bdvd->getDeviceNumber());
  }


  debug(MAIN, "make a deep copy of FsWorkingDir\n");
  main_console->setWorkingDirInfo(new FileSystemInfo(*default_working_dir));
  debug(MAIN, "main_console->setWorkingDirInfo done\n");

  ustl::coutclass::init();
  debug(MAIN, "default_working_dir root name: %s\t pwd name: %s\n",
        default_working_dir->getRoot().dentry_->getName(),
        default_working_dir->getPwd().dentry_->getName());
  if (main_console->getWorkingDirInfo())
  {
    delete main_console->getWorkingDirInfo();
  }
  main_console->setWorkingDirInfo(default_working_dir);

  debug(MAIN, "Timer enable\n");
  ArchInterrupts::enableTimer();

  KeyboardManager::instance();
  ArchInterrupts::enableKBD();

  debug(MAIN, "Adding Kernel threads\n");
  Scheduler::instance()->addNewThread(main_console);
  Scheduler::instance()->addNewThread(new ProcessRegistry(new FileSystemInfo(*default_working_dir), user_progs /*see user_progs.h*/));
  Scheduler::instance()->printThreadList();

  debug(MAIN, "%zu CPU(s) running\n", ArchMulticore::cpu_list_.size());
  kprintf("%zu CPU(s) running\n", ArchMulticore::cpu_list_.size());
  for(auto cls : ArchMulticore::cpu_list_)
  {
          debug(MAIN, "CPU %zu\n", cls->cpu_id);
          kprintf("CPU %zu\n", cls->cpu_id);
  }

  debug(MAIN, "Now enabling Interrupts...\n");
  system_state = RUNNING;


  ArchInterrupts::enableInterrupts();

  Scheduler::instance()->yield();
  //not reached
  assert(false && "Reached end of startup()");
}
