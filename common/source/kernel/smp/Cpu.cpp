#include "Cpu.h"
#include "ArchCpuLocalStorage.h"
#include "debug.h"
#include "assert.h"
#include "DeviceBus.h"

cpu_local size_t Cpu::cpu_id;

Cpu::Cpu() :
    Device("CPU"),
    cpu_id_(&cpu_id)
{
    deviceTreeRoot().addSubDevice(*this);
}

size_t Cpu::id()
{
    return *cpu_id_;
}

void Cpu::setId(size_t id)
{
    *cpu_id_ = id;
    setDeviceName(eastl::string("CPU ") + eastl::to_string(id));
}

void Cpu::enqueueFunctionCallMessage(RemoteFunctionCallMessage* fcall_message)
{
    fcall_queue.pushFront(fcall_message);
}
