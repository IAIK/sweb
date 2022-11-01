#pragma once

#include <cstdint>
#include "EASTL/vector.h"
#include "ACPI.h"

class IOAPIC
{
public:

    enum IOAPICRegisterOffsets
    {
        IOAPICID  =        0x00,
        IOAPICVER =        0x01,
        IOAPICARB =        0x02,
    };

    struct IOAPIC_r_ID
    {
        union
        {
            volatile uint32_t word;
            struct
            {
                volatile uint32_t reserved1  : 24; //  0-23
                volatile uint32_t io_apic_id :  4; // 24-27
                volatile uint32_t reserved2  :  4; // 28-31
            } __attribute__((packed));
        };
    } __attribute__((packed));

    struct IOAPIC_r_VER
    {
        union
        {
            volatile uint32_t word;
            struct
            {
                volatile uint32_t version   : 8; //  0- 7
                volatile uint32_t reserved1 : 8; //  8-15
                volatile uint32_t max_redir : 8; // 16-23
                volatile uint32_t reserved2 : 8; // 24-31
            } __attribute__((packed));
        };
    } __attribute__((packed));

    struct IOAPIC_r_ARB
    {
        union
        {
            volatile uint32_t word;
            struct
            {
                volatile uint32_t reserved1            : 24; // 0-23
                volatile uint32_t arbitration_priority : 4;  // 24-27
                volatile uint32_t reserved2            : 4;  // 27-31
            } __attribute__((packed));
        };
    } __attribute__((packed));

    struct IOAPIC_redir_entry
    {
        union
        {
            volatile uint32_t word_l;
            struct
            {
                volatile uint32_t interrupt_vector : 8; // Allowed values: 0x10-0xFE
                volatile uint32_t delivery_mode    : 3;
                volatile uint32_t destination_mode : 1;
                volatile uint32_t pending_busy     : 1;
                volatile uint32_t polarity         : 1; // 0: active high, 1: active low
                volatile uint32_t lvl_trig_recvd   : 1;
                volatile uint32_t trigger_mode     : 1; // 0: edge, 1: level
                volatile uint32_t mask             : 1;
                volatile uint32_t reserved1        : 15;
            };
        };

        union
        {
            volatile uint32_t word_h;
            struct
            {
                volatile uint32_t reserved2        : 24;
                volatile uint32_t destination      : 8;
            };
        };
    } __attribute__((packed));

    /* These are not actually memory mapped
       struct IOAPICRegisters
       {
       volatile IOAPIC_r_ID r_id;
       volatile IOAPIC_r_VER r_ver;
       volatile IOAPIC_r_ARB r_arb;
       volatile IOAPIC_redir_entry r_redir[0x18];
       } __attribute__((packed));*/

    struct IOAPIC_MMIORegs
    {
        volatile uint32_t io_reg_sel;
        char padding[12];
        volatile uint32_t io_win;
    } __attribute__((packed));



    static void addIOAPIC(uint32_t id, IOAPIC_MMIORegs* regs, uint32_t g_sys_int_base);
    static void addIRQSourceOverride(const MADTInterruptSourceOverride&);
    static void initAll();

    static uint32_t findGSysIntForIRQ(uint8_t irq);

    static void setIRQMask(uint32_t irq_num, bool value);
    static void setGSysIntMask(uint32_t g_sys_int, bool value);

    static IOAPIC* findIOAPICforIRQ(uint8_t irq);
    static IOAPIC* findIOAPICforGlobalInterrupt(uint32_t g_int);


    static eastl::vector<IOAPIC> io_apic_list_;
    static eastl::vector<MADTInterruptSourceOverride> irq_source_override_list_;



    IOAPIC(uint32_t id, IOAPIC_MMIORegs* regs, uint32_t g_sys_int_base);

private:
    void init();
    void initRedirections();

    void mapAt(size_t addr);

    uint32_t read(uint8_t offset);
    void write(uint8_t offset, uint32_t value);

    uint8_t redirEntryOffset(uint32_t entry_no);

    IOAPIC_redir_entry readRedirEntry(uint32_t entry_no);
    void writeRedirEntry(uint32_t entry_no, const IOAPIC_redir_entry& value);

    uint32_t getGlobalInterruptBase();
    uint32_t getMaxRedirEntry();



    static const uint32_t IRQ_OFFSET = 0x20;

    IOAPIC_MMIORegs* reg_paddr_;
    IOAPIC_MMIORegs* reg_vaddr_;

    uint32_t id_;
    uint32_t max_redir_;
    uint32_t g_sys_int_base_;
};
