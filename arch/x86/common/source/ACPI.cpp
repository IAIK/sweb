#include "ACPI.h"
#include "APIC.h"
#include "ArchCommon.h"
#include "ArchMemory.h"
#include "debug.h"
#include "kstring.h"
#include "assert.h"

RSDPDescriptor* RSDP = nullptr;

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

  bool checksum_valid = RSDP->checksumValid();
  if(!checksum_valid)
  {
    assert(false && "Invalid RSDP checksum");
  }


  debug(ACPI, "RSDP checksum valid\n");
  switch(RSDP->Revision)
  {
  case 0:
    debug(ACPI, "RSDP version 1.0\n");
    break;
  case 2:
    debug(ACPI, "RSDP version >=2.0\n");
    break;
  default:
    debug(ACPI, "Invalid RSDP version %x\n", RSDP->Revision);
    assert(false && "Invalid RDSP version");
    break;
  }

  debug(ACPI, "RSDP OEMID: %s\n", RSDP->OEMID);
  debug(ACPI, "RSDT address: %x\n", RSDP->RsdtAddress);


  ACPISDTHeader* RSDT_ptr = (ACPISDTHeader*)(size_t)RSDP->RsdtAddress;
  assert(RSDT_ptr->checksumValid());

  size_t RSDT_entries = RSDT_ptr->numEntries();
  debug(ACPI, "RSDT entrues: %zu\n", RSDT_entries);
  for(size_t i = 0; i < RSDT_entries; ++i)
  {
    ACPISDTHeader* entry_header = RSDT_ptr->getEntry(i);

    {
      char sig[5];
      memcpy(sig, entry_header->Signature, 4);
      sig[4] = '\0';
      debug(ACPI, "[%p] RSDR Header signature: %s\n", entry_header, sig);
    }

    if(memcmp(entry_header->Signature, "APIC", 4) == 0)
    {
      ACPI_MADTHeader* madt = (ACPI_MADTHeader*)entry_header;
      assert(initAPIC(madt));
    }
  }

  /*
  debug(APIC, "APIC spurious interrupt vector: %x, enabled: %x, focus checking: %x\n", apic_vaddr->s_int_vect.vector, apic_vaddr->s_int_vect.enable, apic_vaddr->s_int_vect.focus_checking);
  debug(APIC, "APIC id: %x\n", (apic_vaddr->local_apic_id >> 24));
  apic_vaddr->s_int_vect.enable = 1;
  debug(APIC, "Sending init IPI to APIC ID %x, ICR low: %p, ICR high: %p\n", 0, &apic_vaddr->ICR_low, &apic_vaddr->ICR_high);
  apic_vaddr->ICR_high.target_apic_id = 1;

  {
    APIC_InterruptCommandRegisterLow v_low;
    v_low.vector = 0xF0;
    v_low.destination_mode = 0;
    v_low.delivery_mode = 0b110;
    v_low.destination_shorthand = 0;
    v_low.INIT_level_de_assert_clear = 1;
    apic_vaddr->ICR_low = v_low;
  }
  {
    APIC_InterruptCommandRegisterLow v_low;
    v_low.vector = 0;
    v_low.destination_mode = 0;
    v_low.delivery_mode = 0b101;
    v_low.destination_shorthand = 0;
    v_low.INIT_level_de_assert_clear = 1;
    apic_vaddr->ICR_low = v_low;
    }*/
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

bool ACPISDTHeader::checksumValid()
{
  uint8 sum = 0;
  for(char* i = (char*)this; i < ((char*)this) + Length; ++i)
  {
    sum += *i;
  }
  return sum == 0;
}


size_t ACPISDTHeader::numEntries()
{
  size_t RSDT_entries = (Length - sizeof(*this)) / 4;
  return RSDT_entries;
}

ACPISDTHeader* ACPISDTHeader::getEntry(size_t i)
{
  ACPISDTHeader* entry_ptr = (ACPISDTHeader*)(size_t)(((uint32*)(this + 1))[i]);
  return entry_ptr;
}
