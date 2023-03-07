#pragma once

#include "InterruptDescriptorTable.h"
#include <cinttypes>

constexpr size_t NUM_X86_INT_VECTORS = 256;
constexpr size_t NUM_ISA_INTERRUPTS = 16;

static constexpr uint8_t SYSCALL_INTERRUPT = 0x80;
static constexpr uint8_t YIELD_INTERRUPT = 65;


class InterruptUtils
{
public:
    static InterruptDescriptorTable idt;

private:
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

struct ArchThreadRegisters;

void interruptHandler(size_t interrupt_num, uint64_t error, ArchThreadRegisters* saved_registers);

void irqHandler_0();
void irqHandler_65();
void irqHandler_90();
void irqHandler_91();
void irqHandler_100();
void irqHandler_101();
void irqHandler_127();
void syscallHandler();
void pageFaultHandler(uint64_t address, uint64_t error, uint64_t ip);
void errorHandler(size_t num, size_t eip, size_t cs, size_t spurious);
