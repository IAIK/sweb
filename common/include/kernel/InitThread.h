#pragma once

#include "Thread.h"

class InitThread : public Thread
{
public:
    ~InitThread() override;

    static void init(FileSystemInfo *root_fs_info, char const *progs[]);
    static InitThread* instance();

    void Run() override;

private:
    InitThread(FileSystemInfo *root_fs_info, char const *progs[]);

    static InitThread* instance_;
    char const **progs_;
};
