#ifndef _USERPROCESS_H_
#define _USERPROCESS_H_

#include "Thread.h"

class ProcessRegistry;

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
     *
     */
    UserProcess(const char *minixfs_filename, FileSystemInfo *fs_info, ProcessRegistry *process_registry,
                uint32 terminal_number = 0);

    virtual ~UserProcess();

    virtual void Run(); // not used

  private:

    bool run_me_;
    uint32 terminal_number_;
    int32 fd_;
    ProcessRegistry *process_registry_;
};

#endif
