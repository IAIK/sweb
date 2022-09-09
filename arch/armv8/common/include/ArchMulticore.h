#pragma once

#include "Mutex.h"
#include "EASTL/vector.h"

class CpuInfo
{
public:
    CpuInfo();

    size_t getCpuID();
    void setCpuID(size_t id);

    size_t* cpu_id_;
private:
};

class ArchMulticore
{
public:

    static void initialize();

    static void startOtherCPUs();
    static size_t numRunningCPUs();
    static void stopAllCpus();
    static void stopOtherCpus();


    static void setCurrentCpuId(size_t id);
    static size_t getCurrentCpuId();

    static void initCpu();
    static void initCpuLocalData(bool boot_cpu = false);


    static char* cpuStackTop();

private:
    static void prepareAPStartup(size_t entry_addr);

    [[noreturn]] static void waitForSystemStart();
};
