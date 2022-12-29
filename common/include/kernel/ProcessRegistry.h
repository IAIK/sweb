#pragma once

#include "FileSystemInfo.h"
#include "Mutex.h"
#include "Condition.h"

class ProcessRegistry
{
  public:
    /**
     * Constructor
     * @param root_fs_info the default working directory to be used for new processes
     */
    ProcessRegistry(FileSystemInfo* root_fs_info);
    virtual ~ProcessRegistry() = default;

    static ProcessRegistry* instance();
    static void init(FileSystemInfo* root_fs_info);

    /**
     * Tells us that a userprocess is being destroyed
     */
    void processExit();

    /**
     * Tells us that a userprocess is being created due to a fork or something similar
     */
    void processStart();

    /**
     * Tells us how many processes are running
     */
    size_t processCount();

    void waitAllKilled();

    void createProcess(const char* path);

  private:

    FileSystemInfo* default_working_dir_;
    uint32 progs_running_;
    Mutex counter_lock_;
    Condition all_processes_killed_;
    static ProcessRegistry* instance_;
};
