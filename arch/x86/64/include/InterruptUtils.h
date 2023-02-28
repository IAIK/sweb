#pragma once

#include "types.h"

#include "InterruptDescriptorTable.h"

using handler_func_t = void (*)();

static constexpr uint8_t SYSCALL_INTERRUPT = 0x80;

struct [[gnu::packed]] InterruptHandlers
{
    uint32_t       number;       // handler number
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

union [[gnu::packed]] PagefaultExceptionErrorCode
{
    uint32_t u32;
    struct [[gnu::packed]]
    {
        uint32_t present           :  1;
        uint32_t write             :  1;
        uint32_t user              :  1;
        uint32_t reserved_write    :  1;
        uint32_t instruction_fetch :  1;
        uint32_t protection_key    :  1;
        uint32_t shadow_stack      :  1;
        uint32_t reserved          :  8;
        uint32_t sgx               :  1;
        uint32_t reserved2         : 16;
    };
};

static_assert(sizeof(PagefaultExceptionErrorCode) == sizeof(uint32_t));
