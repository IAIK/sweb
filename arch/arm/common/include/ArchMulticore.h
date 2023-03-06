#pragma once

#include "Mutex.h"
#include "EASTL/vector.h"
#include "ArchCpuLocalStorage.h"
#include "Cpu.h"

class IrqDomain;

class ArchCpu : public Cpu
{
public:
    ArchCpu();

    IrqDomain& rootIrqDomain();
private:
};

class ArchMulticore
{
public:
    static void initialize();

    static void startOtherCPUs();
    /* static void stopAllCpus(); */
    static void stopOtherCpus();

    static void sendFunctionCallMessage(ArchCpu& cpu, RemoteFunctionCallMessage* fcall_message);

    static void initCpuLocalData(bool boot_cpu = false);

private:
};
