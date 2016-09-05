#include "ProcessRegistry.h"
#include "UserProcess.h"
#include "kprintf.h"
#include "Console.h"
#include "Loader.h"
#include "VfsSyscall.h"
#include "File.h"
#include "ArchMemory.h"
#include "PageManager.h"
#include "ArchThreads.h"
#include "offsets.h"

UserProcess::UserProcess(ustl::string filename, FileSystemInfo *fs_info, ProcessRegistry *process_registry,
                         uint32 terminal_number) :
    Thread(fs_info, filename, Thread::USER_THREAD), fd_(VfsSyscall::open(filename, O_RDONLY)), process_registry_(process_registry)
{
  process_registry_->processStart(); //should also be called if you fork a process

  if (fd_ >= 0)
    loader_ = new Loader(fd_);

  if (!loader_ || !loader_->loadExecutableAndInitProcess())
  {
    debug(USERPROCESS, "Error: loading %s failed!\n", filename.c_str());
    loader_ = 0;
    kill();
    return;
  }

  size_t page_for_stack = PageManager::instance()->allocPPN();
  loader_->arch_memory_.mapPage(USER_BREAK / PAGE_SIZE - 1, page_for_stack, 1);

  ArchThreads::createUserRegisters(user_registers_, loader_->getEntryFunction(),
                                   (void*) (USER_BREAK - sizeof(pointer)),
                                   getStackStartPointer());

  ArchThreads::setAddressSpace(this, loader_->arch_memory_);

  debug(USERPROCESS, "ctor: Done loading %s\n", filename.c_str());

  if (main_console->getTerminal(terminal_number))
    setTerminal(main_console->getTerminal(terminal_number));

  switch_to_userspace_ = 1;
}

UserProcess::~UserProcess()
{
  assert(Scheduler::instance()->isCurrentlyCleaningUp());
  delete loader_;
  loader_ = 0;

  if (fd_ > 0)
    vfs_syscall.close(fd_);

  delete working_dir_;
  working_dir_ = 0;

  process_registry_->processExit();
}

void UserProcess::Run()
{
  debug(USERPROCESS, "Run: Fail-safe kernel panic - you probably have forgotten to set switch_to_userspace_ = 1\n");
  assert(false);
}

