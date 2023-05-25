#pragma once

#include "Thread.h"

#include <cstdint>

#include "EASTL/string.h"

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
    UserProcess(const eastl::string& executable_path,
                FileSystemInfo* working_dir,
                uint32_t terminal_number,
                int& creation_status);

    virtual ~UserProcess();

    virtual void Run(); // not used

private:
    int32_t fd_;
};
