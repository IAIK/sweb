#pragma once

#include "Mutex.h"
#include "EASTL/vector.h"
#include "Cpu.h"

class ArchCpu : public Cpu
{
public:
    ArchCpu();
private:
};

class ArchMulticore
{
public:
    static void initialize();

    static void startOtherCPUs();
    static void stopAllCpus();
    static void stopOtherCpus();

    static void sendFunctionCallMessage(ArchCpu& cpu, RemoteFunctionCallMessage* fcall_message);

    static void initCpu();
    static void initCpuLocalData(bool boot_cpu = false);

    static char* cpuStackTop();

private:
    static void prepareAPStartup(size_t entry_addr);

    [[noreturn]] static void waitForSystemStart();
};
