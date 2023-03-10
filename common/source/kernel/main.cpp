#include <ProcessRegistry.h>
#include "InitThread.h"
#include <types.h>
#include "BDManager.h"
#include "BDVirtualDevice.h"
#include "PageManager.h"
#include "ArchInterrupts.h"
#include "ArchThreads.h"
#include "kprintf.h"
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
#include "Terminal.h"
#include "user_progs.h"
#include "ArchMulticore.h"
#include "BlockDeviceInode.h"
#include "BootloaderModules.h"
#include "SystemState.h"
#include "DeviceBus.h"
#include "PlatformBus.h"
#include "libc++.h"

extern void* kernel_end_address;

uint8 boot_stack[0x4000] __attribute__((aligned(0x4000)));
FileSystemInfo* default_working_dir;

extern "C" void removeBootTimeIdentMapping();

void printRunningCpus();
void printInterruptMappings();

extern "C" [[noreturn]] void startup()
{
  system_state = BOOTING;

  debug(MAIN, "Initializing kernel arch mem\n");
  ArchMemory::kernelArchMemory();

  debug(MAIN, "Initializing kernel memory management\n");
  PageManager::init();
  debug(MAIN, "PageManager and KernelMemoryManager created \n");

  debug(MAIN, "Calling global constructors\n");
  _preinit();
  globalConstructors();
  debug(MAIN, "Global constructors finished\n");

  ArchCommon::initKernelVirtualAddressAllocator();

  PlatformBus::initPlatformBus();

  BootloaderModules::mapModules();

  ArchCommon::postBootInit();

  debug(MAIN, "SMP init\n");
  SMP::initialize();

  debug(MAIN, "Interrupts init\n");
  ArchInterrupts::initialise();

  ArchCommon::initPlatformDrivers();

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
  vfs.initialize();
  debug(MAIN, "Mounting root file system\n");
  vfs.registerFileSystem(DeviceFSType::getInstance());
  vfs.registerFileSystem(new RamFSType());
  vfs.registerFileSystem(new MinixFSType());
  default_working_dir = vfs.rootMount("ramfs", 0);
  assert(default_working_dir);

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
  InitThread::init(new FileSystemInfo(*default_working_dir), user_progs /*see user_progs.h*/);
  Scheduler::instance()->addNewThread(InitThread::instance());

  Scheduler::instance()->printThreadList();
  printRunningCpus();
  printInterruptMappings();

  // Ensure we already have a currentThread when interrupts are enabled
  debug(MAIN, "Starting threads and enabling interrupts...\n");
  system_state = RUNNING;

  ArchThreads::startThreads(InitThread::instance());

  // See InitThread::Run() for further startup code

  //not reached
  assert(false && "Reached end of startup()");
}


void printRunningCpus()
{
  debug(MAIN, "%zu CPU(s) running\n", SMP::cpuList().size());
  kprintf("%zu CPU(s) running\n", SMP::cpuList().size());
  for (auto* cpu : SMP::cpuList())
  {
    debug(MAIN, "CPU %zu\n", cpu->id());
    kprintf("CPU %zu\n", cpu->id());
  }
}

void printInterruptMappings()
{
  if (A_INTERRUPTS & OUTPUT_ENABLED)
  {
    for (ArchCpu* cpu : SMP::cpuList())
    {
      debug(MAIN, "CPU %zu IRQ mappings:\n", cpu->id());
      cpu->rootIrqDomain().printAllReverseMappings();
    }
  }
}
