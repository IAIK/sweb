#include "ACPI.h"
#include "APIC.h"
#include "ArchCommon.h"
#include "ArchMemory.h"
#include "debug.h"
#include "kstring.h"
#include "assert.h"
#include "new.h"

RSDPDescriptor* RSDP = nullptr;
size_t ACPI_version = 0;

RSDPDescriptor* checkForRSDP(char* start, char* end)
{
  debug(ACPI, "Search segment [%p, %p) for RSDP\n", start, end);
  for(char* i = (char*)start; i < (char*)end; ++i)
  {
    if(memcmp(i, "RSD PTR ", 8) == 0)
    {
      debug(ACPI, "Found RSDP at %p\n", i);
      return (RSDPDescriptor*)i;
    }
  }
  return nullptr;
}

RSDPDescriptor* locateRSDP()
{
  debug(ACPI, "locateRSDP\n");
  size_t num_mmaps = ArchCommon::getNumUseableMemoryRegions();
  for (size_t i = 0; i < num_mmaps; ++i)
  {
    pointer start_address = 0, end_address = 0, type = 0;
    ArchCommon::getUsableMemoryRegion(i, start_address, end_address, type);
    if(end_address == 0)
      end_address = 0xFFFFFFFF; // TODO: Fix this (use full 64 bit addresses for memory detection)
    if((type == 2) && (end_address <= 0x100000))
    {
      RSDPDescriptor* found = checkForRSDP((char*)start_address, (char*)end_address);
      if(found)
      {
        return found;
      }
    }
  }
  return nullptr;
}



void initACPI()
{
  debug(ACPI, "initACPI\n");

  RSDP = locateRSDP();
  assert(RSDP && "Could not find RSDP");

  assert(RSDP->checksumValid());
  ACPI_version = (RSDP->Revision == 0 ? 1 : RSDP->Revision);
  debug(ACPI, "ACPI version: %zu\n", ACPI_version);
  debug(ACPI, "RSDP OEMID: %s\n", RSDP->OEMID);

  switch(RSDP->Revision)
  {
  case 0:
  {
    debug(ACPI, "RSDT address: %x\n", RSDP->RsdtAddress);

    RSDT* RSDT_ptr = (RSDT*)(size_t)RSDP->RsdtAddress;
    //ACPISDTHeader* RSDT_ptr = (ACPISDTHeader*)(size_t)RSDP->RsdtAddress;
    assert(RSDT_ptr->h.checksumValid());

    size_t RSDT_entries = RSDT_ptr->numEntries();
    debug(ACPI, "RSDT entries: %zu\n", RSDT_entries);
    for(size_t i = 0; i < RSDT_entries; ++i)
    {
            handleSDT(RSDT_ptr->getEntry(i));
    }

    break;
  }
  case 2:
  {
    RSDPDescriptor20* RSDP2 = (RSDPDescriptor20*)RSDP;
    assert(RSDP2->checksumValid());

    debug(ACPI, "XSDT address: %lx\n", RSDP2->XsdtAddress);

    XSDT* XSDT_ptr = (XSDT*)(size_t)RSDP2->XsdtAddress;
    //ACPISDTHeader* XSDT_ptr = (ACPISDTHeader*)(size_t)RSDP2->XsdtAddress;
    assert(XSDT_ptr->h.checksumValid());

    size_t XSDT_entries = XSDT_ptr->numEntries();
    debug(ACPI, "XSDT entrues: %zu\n", XSDT_entries);
    for(size_t i = 0; i < XSDT_entries; ++i)
    {
            handleSDT(XSDT_ptr->getEntry(i));
    }

    break;
  }
  default:
    debug(ACPI, "Invalid RSDP version %x\n", RSDP->Revision);
    assert(false && "Invalid RDSP version");
    break;
  }
}

void handleSDT(ACPISDTHeader* entry_header)
{
        {
                char sig[5];
                memcpy(sig, entry_header->Signature, 4);
                sig[4] = '\0';
                debug(ACPI, "[%p] RSDR Header signature: %s\n", entry_header, sig);
        }

        if(memcmp(entry_header->Signature, "APIC", 4) == 0)
        {
                ACPI_MADTHeader* madt = (ACPI_MADTHeader*)entry_header;
                madt->parse();
        }
}


bool RSDPDescriptor::checksumValid()
{
  uint8 sum = 0;
  for(char* i = (char*)this; i < ((char*)this) + sizeof(*this); ++i)
  {
    sum += *i;
  }
  debug(ACPI, "RSDP checksum %x\n", sum);
  return sum == 0;
}

bool RSDPDescriptor20::checksumValid()
{
        uint8 sum = 0;
        for(char* i = (char*)this; i < ((char*)this) + sizeof(*this); ++i)
        {
                sum += *i;
        }
        debug(ACPI, "RSDP 2.0 checksum %x\n", sum);
        return sum == 0;
}

bool ACPISDTHeader::checksumValid()
{
  uint8 sum = 0;
  for(char* i = (char*)this; i < ((char*)this) + Length; ++i)
  {
    sum += *i;
  }
  return sum == 0;
}


size_t RSDT::numEntries()
{
        size_t RSDT_entries = (h.Length - sizeof(*this)) / 4;
        return RSDT_entries;
}

size_t XSDT::numEntries()
{
        size_t XSDT_entries = (h.Length - sizeof(*this)) / 8;
        return XSDT_entries;
}

ACPISDTHeader* RSDT::getEntry(size_t i)
{
        ACPISDTHeader* entry_ptr = (ACPISDTHeader*)(size_t)(((uint32*)(this + 1))[i]);
        return entry_ptr;
}

ACPISDTHeader* XSDT::getEntry(size_t i)
{
        ACPISDTHeader* entry_ptr = (ACPISDTHeader*)(size_t)(((uint64*)(this + 1))[i]);
        return entry_ptr;
}

void ACPI_MADTHeader::parse()
{
  if(!LocalAPIC::initialized)
  {
          new (&local_APIC) LocalAPIC(this);
  }

  MADTEntryDescriptor* madt_entry = (MADTEntryDescriptor*)(this + 1);
  while((size_t)madt_entry < (size_t)this + std_header.Length)
  {
    switch(madt_entry->type)
    {
    case 0:
    {
      MADTProcLocalAPIC* entry = (MADTProcLocalAPIC*)(madt_entry + 1);
      debug(ACPI, "[%p] Processor local APIC, ACPI Processor ID: %4x, APIC ID: %4x, enabled: %u\n", entry, entry->proc_id, entry->apic_id, entry->flags.enabled);
      local_APIC.addLocalAPICToList(*entry);
      break;
    }
    case 1:
    {
      MADT_IO_APIC* entry = (MADT_IO_APIC*)(madt_entry + 1);
      debug(ACPI, "[%p] I/O APIC, id: %x, address: %x, g_sys_int base: %x\n", entry, entry->id, entry->address, entry->global_system_interrupt_base);
      if(!IOAPIC::initialized)
      {
              new (&IO_APIC) IOAPIC(entry->id, (IOAPIC::IOAPIC_MMIORegs*)(size_t)entry->address, entry->global_system_interrupt_base);
      }
      break;
    }
    case 2:
    {
      MADTInterruptSourceOverride* entry = (MADTInterruptSourceOverride*)(madt_entry + 1);
      debug(ACPI, "[%p] Interrupt Source Override, bus_source: %x, irq_source: %3x, g_sys_int: %3x, polarity: %x, trigger mode: %x\n", entry, entry->bus_source, entry->irq_source, entry->g_sys_int, entry->flags.polarity, entry->flags.trigger_mode);
      IO_APIC.addIRQSourceOverride(*entry);
      break;
    }
    case 3:
    {
      MADTNonMaskableInterruptsSource* entry = (MADTNonMaskableInterruptsSource*)(madt_entry + 1);
      debug(ACPI, "[%p] NMI source, g_sys_int: %x, flags: %x\n", entry, entry->g_sys_int, entry->flags);
      break;
    }
    case 4:
    {
      MADTNonMaskableInterrupts* entry = (MADTNonMaskableInterrupts*)(madt_entry + 1);
      debug(ACPI, "[%p] Local APIC NMI, proc_id: %x, flags: %x, lint_num: %x\n", entry, entry->processor_id, entry->flags, entry->lint_num);
      break;
    }
    case 5:
    {
      MADTLocalAPICAddressOverride* entry = (MADTLocalAPICAddressOverride*)(madt_entry + 1);
      debug(ACPI, "[%p] Local APIC address override, addr: %zx\n", entry, entry->local_apic_addr);
      assert(LocalAPIC::initialized);
      local_APIC.reg_paddr_ = (LocalAPICRegisters*)entry->local_apic_addr;
      break;
    }
    case 9:
    {
      MADTProcLocalx2APIC* entry = (MADTProcLocalx2APIC*)(madt_entry + 1);
      debug(ACPI, "[%p] Processor local x2APIC, x2APIC ID: %4x, enabled: %u, proc UID: %x\n", entry, entry->x2apic_id, entry->flags.enabled, entry->processor_uid);
      break;
    }
    default:
      debug(ACPI, "[%p] Unknown MADT entry type %x\n", madt_entry + 1, madt_entry->type);
      break;
    }
    madt_entry = (MADTEntryDescriptor*)((size_t)madt_entry + madt_entry->length);
  }
}


void LocalAPIC::addLocalAPICToList(const MADTProcLocalAPIC& entry)
{
        assert(LocalAPIC::initialized);
        local_apic_list_.push_back(entry);
}


void IOAPIC::addIRQSourceOverride(const MADTInterruptSourceOverride& entry)
{
        assert(IOAPIC::initialized);
        irq_source_override_list_.push_back(entry);
}
