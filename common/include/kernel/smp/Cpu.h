#pragma once

#include "AtomicMpScQueue.h"
#include "Device.h"
#include "RemoteFunctionCall.h"

#include "ArchCpuLocalStorage.h"

#include <cstddef>

// Information concerning one cpu in the system
// Base functionality for all architectures
class Cpu : public Device
{
public:
    Cpu();
    Cpu(const Cpu&) = delete;
    Cpu& operator=(const Cpu&) = delete;

    size_t id();
    void setId(size_t id);

    void enqueueFunctionCallMessage(RemoteFunctionCallMessage* fcall_message);

    AtomicMpScQueue<RemoteFunctionCallMessage> fcall_queue;

private:
    // pointer to thread local cpu_id variable so we can read the id from other cpus
    size_t* cpu_id_;

    cpu_local static size_t cpu_id;
};
