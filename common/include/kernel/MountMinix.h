/**
 * @file MountMinix.h
 */

#ifndef _MOUNTMINIX_H_
#define _MOUNTMINIX_H_

#include "Thread.h"
#include "Mutex.h"
#include "Condition.h"

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
     * @param progs a string-array of the userprograms which should be executed
     */
    MountMinixAndStartUserProgramsThread ( FileSystemInfo *root_fs_info, char const *progs[] );

    /**
     * Mounts the Minix-Partition with user-programs and creates processes
     */
    virtual void Run();

    /**
     * Tells us that a userprocess is being destroyed
     */
    void processExit();

    /**
     * Tells us that a userprocess is being created due to a fork or something similar
     */
    void processStart();

  private:

    char const **progs_;
    uint32 progs_running_;
    Mutex counter_lock_;
    Condition all_processes_killed_;
};

#endif
