#pragma once

#include "types.h"

struct RSDPDescriptor {
        char Signature[8];
        uint8 Checksum;
        char OEMID[6];
        uint8 Revision;
        uint32 RsdtAddress;

        bool checksumValid();
} __attribute__ ((packed));

struct RSDPDescriptor20 {
        RSDPDescriptor firstPart;

        uint32 Length;
        uint64 XsdtAddress;
        uint8 ExtendedChecksum;
        uint8 reserved[3];

        bool checksumValid();
} __attribute__ ((packed));


struct ACPISDTHeader {
        char Signature[4];
        uint32 Length;
        uint8 Revision;
        uint8 Checksum;
        char OEMID[6];
        char OEMTableID[8];
        uint32 OEMRevision;
        uint32 CreatorID;
        uint32 CreatorRevision;

        bool checksumValid();
} __attribute__ ((packed));

struct RSDT
{
        ACPISDTHeader h;

        size_t numEntries();
        ACPISDTHeader* getEntry(size_t i);
};

struct XSDT
{
        ACPISDTHeader h;

        size_t numEntries();
        ACPISDTHeader* getEntry(size_t i);
};

struct MADTExtendedHeader
{
        uint32 local_apic_addr;
        uint32 flags;
} __attribute__ ((packed));

struct ACPI_MADTHeader
{
        ACPISDTHeader std_header;
        MADTExtendedHeader ext_header;

        void parse();
} __attribute__ ((packed));

struct MADTEntryDescriptor
{
        uint8 type;
        uint8 length;
} __attribute__ ((packed));

struct MADTProcLocalAPIC
{
        uint8 proc_id;
        uint8 apic_id;
        struct
        {
                uint32 enabled  :  1;
                uint32 reserved : 31;
        } __attribute__ ((packed)) flags;
} __attribute__ ((packed));

struct MADT_IO_APIC
{
        uint8 id;
        uint8 reserved;
        uint32 address;
        uint32 global_system_interrupt_base;
} __attribute__ ((packed));

struct MADTInterruptSourceOverride
{
        uint8 bus_source;
        uint8 irq_source;
        uint32 g_sys_int;
        struct
        {
                uint16 polarity     : 2; // 00: conforms to bus specifications, 01: active high, 10: reserved, 11: active low
                uint16 trigger_mode : 2; // 00: conforms to bus specifications, 01: edge triggered, 10: reserved, 11: level triggered
                uint16 reserved     : 12;
        } __attribute__ ((packed)) flags;
} __attribute__ ((packed));

enum
{
        ACPI_MADT_POLARITY_CONFORMS    = 0,
        ACPI_MADT_POLARITY_ACTIVE_HIGH = 1,
        ACPI_MADT_POLARITY_RESERVED    = 2,
        ACPI_MADT_POLARITY_ACTIVE_LOW  = 3,
};

enum
{
        ACPI_MADT_TRIGGER_CONFORMS = 0,
        ACPI_MADT_TRIGGER_EDGE     = 1,
        ACPI_MADT_TRIGGER_RESERVED = 2,
        ACPI_MADT_TRIGGER_LEVEL    = 3,
};

struct MADTNonMaskableInterruptsSource
{
        uint16 flags;
        uint32 g_sys_int;
} __attribute__ ((packed));

struct MADTNonMaskableInterrupts
{
        uint8 processor_id;
        uint16 flags;
        uint8 lint_num;
} __attribute__ ((packed));

struct MADTLocalAPICAddressOverride
{
        uint16 reserved;
        uint64 local_apic_addr;
} __attribute__ ((packed));

struct MADTProcLocalx2APIC
{
        uint16 reserved;
        uint32 x2apic_id;
        struct
        {
                uint32 enabled  :  1;
                uint32 reserved : 31;
        } __attribute__ ((packed)) flags;
        uint32 processor_uid;
} __attribute__ ((packed));



void initACPI();
RSDPDescriptor* checkForRSDP(char* start, char* end);
RSDPDescriptor* locateRSDP();

void handleSDT(ACPISDTHeader*);

extern RSDPDescriptor* RSDP;
extern size_t ACPI_version;
