#pragma once

#include "InterruptDescriptorTable.h"

#include <cinttypes>

#define DPL_KERNEL_SPACE     0 // kernelspace's protection level
#define DPL_USER_SPACE       3 // userspaces's protection level

namespace InterruptVector
{
    static constexpr size_t NUM_VECTORS = 256;
    static constexpr size_t NUM_ISA_INTERRUPTS = 16;

    static constexpr uint8_t REMAP_OFFSET = 32;

    static constexpr uint8_t YIELD            = 65;
    static constexpr uint8_t IPI_HALT_CPU     = 90;
    static constexpr uint8_t APIC_ERROR       = 91;
    static constexpr uint8_t APIC_SPURIOUS    = 100;
    static constexpr uint8_t IPI_REMOTE_FCALL = 101;
    static constexpr uint8_t APIC_TIMER       = 127;
    static constexpr uint8_t SYSCALL          = 0x80;
};

// Standard ISA IRQs
enum class ISA_IRQ : uint8_t
{
    PIT              = 0, // 0 	Programmable Interrupt Timer Interrupt
    KEYBOARD         = 1, // 1 	Keyboard Interrupt
    PIC_8259_CASCADE = 2, // 2 	Cascade (used internally by the two PICs. never raised)
    COM2             = 3, // 3 	COM2 (if enabled)
    COM1             = 4, // 4 	COM1 (if enabled)
    LPT2             = 5, // 5 	LPT2 (if enabled)
    FLOPPY_DISK      = 6, // 6 	Floppy Disk
    LPT1             = 7, // 7 	LPT1 / Unreliable "spurious" interrupt (usually)
    RTC              = 8, // 8 	CMOS real-time clock (if enabled)
    // 9 	Free for peripherals / legacy SCSI / NIC
    // 10 	Free for peripherals / SCSI / NIC
    // 11 	Free for peripherals / SCSI / NIC
    PS2_MOUSE        = 12, // 12 	PS2 Mouse
    FPU              = 13, // 13 	FPU / Coprocessor / Inter-processor
    ATA_PRIMARY      = 14, // 14 	Primary ATA Hard Disk
    ATA_SECONDARY    = 15  // 15 	Secondary ATA Hard Disk
};

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

struct ArchThreadRegisters;

void interruptHandler(size_t interrupt_num, uint32_t error_code, ArchThreadRegisters* saved_registers);

void int32_handler_PIT_irq0();
void int65_handler_swi_yield();
void int90_handler_halt_cpu();
void int91_handler_APIC_error();
void int100_handler_APIC_spurious();
void int101_handler_cpu_fcall();
void int127_handler_APIC_timer();
void syscallHandler();
void pageFaultHandler(uint32_t address, uint32_t error, uint32_t ip);
void errorHandler(size_t num, size_t eip, size_t cs, size_t spurious);
