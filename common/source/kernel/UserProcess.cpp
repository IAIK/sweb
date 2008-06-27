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
  if(loader_->loadExecutableAndInitProcess())
  {
    run_me_ = true;
    kprintfd ( "UserProcess::ctor: Done loading %s\n",minixfs_filename );
  }
}

UserProcess::~UserProcess()
{
  vfs_syscall.close(fd_);
  MountMinixAndStartUserProgramsThread::process_exit();
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
    currentThread->kill();
}


///////////////////////////////////////////////////////////////////////

uint32 MountMinixAndStartUserProgramsThread::prog_counter_(0);
Mutex MountMinixAndStartUserProgramsThread::lock_;
Thread *MountMinixAndStartUserProgramsThread::unmount_thread_(0);

class UnmountMinixThread : public Thread
{
  public:
    /**
     * Constructor
     * @param root_fs_info the FileSystemInfo
     */
    UnmountMinixThread ( FileSystemInfo *root_fs_info ) :
      Thread ( root_fs_info, "UnmountMinixThread" )
    {
    }

    /**
     * Unmounts the Minix-Partition with user-programs
     */
    virtual void Run()
    {
      vfs_syscall.umount ( "/user_progs", 0 );
    }
};

void MountMinixAndStartUserProgramsThread::Run()
{
  if(!progs_)
    return;

  vfs_syscall.mkdir ( "/user_progs", 0 );
  vfs_syscall.mount ( "idea1", "/user_progs", "minixfs", 0 );

  unmount_thread_ = new UnmountMinixThread ( new FileSystemInfo ( *fs_info_ )) ;

  lock_.acquire();

  while(progs_[prog_counter_])
    Scheduler::instance()->addNewThread (new UserProcess ( progs_[prog_counter_++], new FileSystemInfo ( *fs_info_ )) );

  lock_.release();

  state_ = ToBeDestroyed;
  Scheduler::instance()->yield();
}

void MountMinixAndStartUserProgramsThread::process_exit()
{
  MutexLock lock(lock_);

  if(--prog_counter_ == 0)
    Scheduler::instance()->addNewThread (unmount_thread_);
}

