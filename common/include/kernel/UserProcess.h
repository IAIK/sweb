/**
 * @file UserProcess.h
 */

#ifndef _USERPROCESS_H_
#define _USERPROCESS_H_

#include "Thread.h"

/**
 * @class UserProcess
 * Thread used to execute a file in minixfs.
 */
class UserProcess : public Thread
{
  public:
    /**
     * Constructor
     * @param minixfs_filename filename of the file in minixfs to execute
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

