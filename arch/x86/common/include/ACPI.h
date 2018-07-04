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
        size_t numEntries();
        ACPISDTHeader* getEntry(size_t i);
} __attribute__ ((packed));

struct MADTExtendedHeader
{
        uint32 local_apic_addr;
        uint32 flags;
} __attribute__ ((packed));

struct ACPI_MADTHeader
{
        ACPISDTHeader std_header;
        MADTExtendedHeader ext_header;
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
        uint32 flags;
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
        uint32 global_system_interrupt;
        uint16 flags;
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



void initACPI();
RSDPDescriptor* checkForRSDP(char* start, char* end);
RSDPDescriptor* locateRSDP();

extern RSDPDescriptor* RSDP;
