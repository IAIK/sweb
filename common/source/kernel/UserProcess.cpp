/**
 * @file UserProcess.cpp
 */

#include <ProcessRegistry.h>
#include "UserProcess.h"
#include "kprintf.h"
#include "Console.h"
#include "Loader.h"
#include "VfsSyscall.h"
#include "fs/File.h"

UserProcess::UserProcess(const char *minixfs_filename, FileSystemInfo *fs_info, ProcessRegistry *process_registry,
                         uint32 terminal_number) :
    Thread(fs_info, minixfs_filename), run_me_(false), terminal_number_(terminal_number),
    fd_(VfsSyscall::open(minixfs_filename, O_RDONLY)), process_registry_(process_registry)
{
  process_registry_->processStart(); //should also be called if you fork a process

  if (fd_ < 0)
  {
    debug(USERPROCESS, "Error: file %s does not exist!\n", minixfs_filename);
    loader_ = 0;
    kill();
    return;
  }

  loader_ = new Loader(fd_, this);
  if (loader_ && loader_->loadExecutableAndInitProcess())
  {
    run_me_ = true;
    debug(USERPROCESS, "ctor: Done loading %s\n", minixfs_filename);
  }

  if (main_console->getTerminal(terminal_number_))
    setTerminal(main_console->getTerminal(terminal_number_));

  switch_to_userspace_ = 1;
}

extern VfsSyscall vfs_syscall;

UserProcess::~UserProcess()
{
  if (fd_ > 0)
    vfs_syscall.close(fd_);

  process_registry_->processExit();
}

void UserProcess::Run()
{
  debug(USERPROCESS,
        "UserProcess::Run: Fail-safe kernel panic - you probably have forgotten to set switch_to_userspace_ = 1\n");
  assert(false);
}

