#pragma once

#include "Thread.h"

class InitThread : public Thread
{
public:
    InitThread(FileSystemInfo *root_fs_info, char const *progs[]);
    virtual ~InitThread();

    virtual void Run();
private:
    char const **progs_;
};
