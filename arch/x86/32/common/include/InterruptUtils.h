#pragma once

#include "InterruptDescriptorTable.h"
#include "types.h"

using handler_func_t = void (*)();

#define DPL_KERNEL_SPACE     0 // kernelspace's protection level
#define DPL_USER_SPACE       3 // userspaces's protection level

static constexpr uint8_t SYSCALL_INTERRUPT = 0x80;

struct [[gnu::packed]] InterruptHandlers
{
    uint32_t number;    // handler number
    handler_func_t handler_func; // pointer to handler function
};

class InterruptUtils
{
public:
    static void initialise();

    static InterruptDescriptorTable idt;

private:
    static InterruptHandlers handlers[];
};
