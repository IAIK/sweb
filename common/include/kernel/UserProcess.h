/**
 * @file UserProcess.h
 */

#ifndef _USERPROCESS_H_
#define _USERPROCESS_H_

#include "Thread.h"
#include "Mutex.h"

/**
 * @class MountMinixAndStartUserProgramsThread
 * Helper thread which mounts the second partition and starts the
 * selected userprograms on it
 */
class MountMinixAndStartUserProgramsThread : public Thread
{
  public:
    /**
     * Constructor
     * @param root_fs_info the FileSystemInfo
     */
    MountMinixAndStartUserProgramsThread ( FileSystemInfo *root_fs_info, char const *progs[] ) :
      Thread ( root_fs_info, "MountMinixAndStartUserProgramsThread" ), progs_(progs)
    {
    }

    /**
     * Mounts the Minix-Partition with user-programs and creates processes
     */
    virtual void Run();

    static void process_exit();

  private:

    char const **progs_;
    static uint32 prog_counter_;
    static Mutex lock_;
    static Thread *unmount_thread_;
};

/**
 * @class UserProcess
 * Thread used to execute a file in minixfs.
 * NOTE: this process is only ONE thread; if you want to implement mulithreading,
 * each thread has to be inherited from @ref Thread
 */
class UserProcess : public Thread
{
  public:
    /**
     * Constructor
     * @param minixfs_filename filename of the file in minixfs to execute
     * @param fs_info filesysteminfo-object to be used
     * @param terminal_number the terminal to run in (default 0)
     */
    UserProcess ( const char *minixfs_filename, FileSystemInfo *fs_info, uint32 terminal_number = 0 );

    /**
     * Destructor
     */
    virtual ~UserProcess ();

    /**
     * Starts the process
     */
    virtual void Run();

  private:
    bool run_me_;
    uint32 terminal_number_;
    int32 fd_;
};


#endif

