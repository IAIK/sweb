#pragma once

#include "Thread.h"

class UserProcess : public Thread
{
  public:
    /**
     * Constructor
     * @param executable_path path to the file to execute
     * @param working_dir working directory for the process
     * @param terminal_number the terminal to run in (default 0)
     *
     */
    UserProcess(eastl::string executable_path, FileSystemInfo* working_dir, uint32 terminal_number = 0);

    virtual ~UserProcess();

    virtual void Run(); // not used

  private:
    int32 fd_;
};
