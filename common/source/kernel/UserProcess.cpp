/**
 * @file UserProcess.cpp
 */

#include "UserProcess.h"
#include "fs/fs_global.h"
#include "console/kprintf.h"
#include "console/Console.h"
#include "Loader.h"

UserProcess::UserProcess ( const char *minixfs_filename, FileSystemInfo *fs_info,
                           uint32 terminal_number ) :
  Thread ( fs_info, minixfs_filename ),
  run_me_(false),
  terminal_number_(terminal_number),
  fd_(vfs_syscall.open ( minixfs_filename, O_RDONLY ) )
{
  if ( fd_ < 0 )
  {
    kprintfd ( "Error: file %s does not exist!\n",minixfs_filename );
    loader_ = 0;
    return;
  }

  loader_= new Loader ( fd_, this );

  if(loader_ && loader_->loadExecutableAndInitProcess())
  {
    run_me_ = true;
    kprintfd ( "UserProcess::ctor: Done loading %s\n",minixfs_filename );
  }
}

UserProcess::~UserProcess()
{
  vfs_syscall.close(fd_);
}

void UserProcess::Run()
{
  if ( run_me_ )
    for ( ;; )
    {
      if ( main_console->getTerminal ( terminal_number_ ) )
        setTerminal ( main_console->getTerminal ( terminal_number_ ) );
      kprintfd ( "UserProcess:Run: %x  %d:%s Going to user, expect page fault\n", this, getPID(), getName() );
      switch_to_userspace_ = 1;
      Scheduler::instance()->yield();
      //should not reach
    }
  else
    kill();
}

