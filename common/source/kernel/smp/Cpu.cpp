#include "Cpu.h"
#include "ArchCpuLocalStorage.h"
#include "debug.h"
#include "assert.h"

cpu_local size_t Cpu::cpu_id;

Cpu::Cpu() :
    cpu_id_(&cpu_id)
{

}

size_t Cpu::id()
{
    return *cpu_id_;
}

void Cpu::setId(size_t id)
{
    *cpu_id_ = id;
}

void Cpu::enqueueFunctionCallMessage(RemoteFunctionCallMessage* fcall_message)
{
    fcall_queue.pushBack(fcall_message);
}
