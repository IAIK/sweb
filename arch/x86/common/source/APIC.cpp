#include "APIC.h"
#include "debug.h"

LocalAPICRegisters* local_APIC = nullptr;

LocalAPICRegisters* initAPIC(ACPI_MADTHeader* madt)
{
  debug(APIC, "initAPIC, MADT: %p\n", madt);
  local_APIC = (LocalAPICRegisters*)(size_t)madt->ext_header.local_apic_addr;
  debug(APIC, "MADT: Local APIC addr: %p, flags: %x\n", local_APIC, madt->ext_header.flags);

  MADTEntryDescriptor* madt_entry = (MADTEntryDescriptor*)(madt + 1);
  while((size_t)madt_entry < (size_t)madt + madt->std_header.Length)
  {
    switch(madt_entry->type)
    {
    case 0:
    {
      MADTProcLocalAPIC* entry = (MADTProcLocalAPIC*)(madt_entry + 1);
      debug(APIC, "[%p] Processor local APIC, ACPI Processor ID: %4x, APIC ID: %4x, flags: %x\n", entry, entry->proc_id, entry->apic_id, entry->flags);
      break;
    }
    case 1:
    {
      MADT_IO_APIC* entry = (MADT_IO_APIC*)(madt_entry + 1);
      debug(APIC, "[%p] I/O APIC, id: %x, address: %x, g_sys_int base: %x\n", entry, entry->id, entry->address, entry->global_system_interrupt_base);
      break;
    }
    case 2:
    {
      MADTInterruptSourceOverride* entry = (MADTInterruptSourceOverride*)(madt_entry + 1);
      debug(APIC, "[%p] Interrupt Source Override, bus_source: %x, irq_source: %3x, g_sys_int: %3x, flags: %3x\n", entry, entry->bus_source, entry->irq_source, entry->global_system_interrupt, entry->flags);
      break;
    }
    case 4:
    {
      MADTNonMaskableInterrupts* entry = (MADTNonMaskableInterrupts*)(madt_entry + 1);
      debug(APIC, "[%p] Non maskable interrupts, proc_id: %x, flags: %x, lint_num: %x\n", entry, entry->processor_id, entry->flags, entry->lint_num);
      break;
    }
    case 5:
    {
      MADTLocalAPICAddressOverride* entry = (MADTLocalAPICAddressOverride*)(madt_entry + 1);
      debug(APIC, "[%p] Local APIC address override, addr: %zx\n", entry, entry->local_apic_addr);
      break;
    }
    default:
      debug(APIC, "[%p] Unknown MADT entry type %x\n", madt_entry + 1, madt_entry->type);
      break;
    }
    madt_entry = (MADTEntryDescriptor*)((size_t)madt_entry + madt_entry->length);
  }

  return local_APIC;
}
