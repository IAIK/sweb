#pragma once

#include "types.h"
#include "ACPI.h"

struct APIC_InterruptCommandRegisterLow
{
        uint32 vector                     : 8;  // 0-7
        uint32 delivery_mode              : 3;  // 8-10
        uint32 destination_mode           : 1;  // 11
        uint32 delivery_status            : 1;  // 12
        uint32 reserved                   : 1;  // 13
        uint32 INIT_level_de_assert_clear : 1;  // 14
        uint32 INIT_level_de_assert_set   : 1;  // 15
        uint32 reserved2                  : 2;  // 16-17
        uint32 destination_shorthand      : 2;  // 18-19
        uint32 reserved3                  : 12; // 20-31
} __attribute__ ((packed));

struct APIC_InterruptCommandRegisterHigh
{
        uint32 reserved       : 24; //  0-23
        uint32 target_apic_id : 4;  // 24-27
        uint32 reserved2      : 4;  // 28-31
} __attribute__ ((packed));

struct APIC_SpuriousInterruptVector
{
        uint32 one            : 4;
        uint32 vector         : 4;
        uint32 enable         : 1;
        uint32 focus_checking : 1;
        uint32 reserved       : 22;
} __attribute__ ((packed));



struct LocalAPICRegisters
{
        char reserved1[0x10*2]; // 0-1

        uint32 local_apic_id; // 2
        char padding1[12];

        const uint32 local_apic_id_version; // 3
        char padding2[12];

        char reserved2[0x10*4]; // 4-7

        uint32 task_priority; // 8
        char padding3[12];

        uint32 arbitration_priority; // 9
        char padding4[12];

        uint32 processor_priority; // 10
        char padding5[12];

        uint32 eoi; // 11
        char padding6[12];

        char reserved[0x10]; // 12

        uint32 logical_destination; // 13
        char padding7[12];

        uint32 destination_format; // 14
        char padding8[12];

        APIC_SpuriousInterruptVector s_int_vect; // 15
        char padding9[12];

        struct
        {
                uint32 isr;
                char padding[12];
        }ISR[8] __attribute__((packed)); //

        struct
        {
                uint32 tmr;
                char padding[12];
        }TMR[8] __attribute__((packed));

        struct
        {
                uint32 irr;
                char padding[12];
        }IRR[8] __attribute__((packed));

        uint32 error_status;
        char padding10[12];

        char reserved3[0x10*7];

        APIC_InterruptCommandRegisterLow ICR_low;
        char padding11[12];

        APIC_InterruptCommandRegisterHigh ICR_high;
        char padding12[12];

        uint32 local_vector_table;
        char padding13[12];

        char reserved4[0x10];

        uint32 performance_counter;
        char padding14[12];

        uint32 lint0;
        char padding15[12];

        uint32 lint1;
        char padding16[12];

        uint32 lvt_error;
        char padding17[12];

        uint32 init_timer_count;
        char padding18[12];

        uint32 current_timer_count;
        char padding19[12];

        char reserved5[0x10*4];

        uint32 timer_divide_config;
        char padding20[12];

        char reserved6[0x10];
} __attribute__((packed));

static_assert(sizeof(LocalAPICRegisters) == 0x400);


extern LocalAPICRegisters* local_APIC;


LocalAPICRegisters* initAPIC(ACPI_MADTHeader* madt);
