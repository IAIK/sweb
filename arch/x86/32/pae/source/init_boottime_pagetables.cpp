#include "kprintf.h"
#include "multiboot.h"
#include "offsets.h"
#include "paging-definitions.h"

#include "ArchCommon.h"
#include "ArchMemory.h"

#include "types.h"

#include "assert.h"

extern void* kernel_end_address;

extern "C" void initialiseBootTimePaging()
{
  uint32 i;

  PageDirPointerTableEntry *pdpt_start = (PageDirPointerTableEntry*) VIRTUAL_TO_PHYSICAL_BOOT(
      (pointer )kernel_page_directory_pointer_table);
  PageDirEntry *pde_start = (PageDirEntry*) VIRTUAL_TO_PHYSICAL_BOOT((pointer )kernel_page_directory);

  VAddr k_start{ArchCommon::getKernelStartAddress()};

  for (i = 0; i < 4; ++i)
  {
    pdpt_start[i].page_ppn = ((uint32) pde_start) / PAGE_SIZE + i;
    pdpt_start[i].present = 1;
  }

  PageTableEntry *pte_start = (PageTableEntry*) VIRTUAL_TO_PHYSICAL_BOOT((pointer )kernel_page_tables);


  // we do not have to clear the pde since its in the bss
  for (i = 0; i < 10; ++i)
  {
    pde_start[i].page.page_ppn = i;
    pde_start[i].page.writeable = 1;
    pde_start[i].page.size = 1;
    pde_start[i].page.present = 1;
  }

  for (i = 0; i < 8; ++i)
  {
    pde_start[i + 1024].pt.present = 1;
    pde_start[i + 1024].pt.writeable = 1;
    pde_start[i + 1024].pt.page_ppn = ((pointer) &pte_start[PAGE_TABLE_ENTRIES * i]) / PAGE_SIZE;
  }

  // ok, we currently only fill in mappings for the first 4 megs (aka one page table)
  // we do not have to zero out the other page tables since they're already empty
  // thanks to the bss clearance.

  // update, from now on, all pages up to the last page containing only rodata
  // will be write protected.

  extern uint32 text_start_address;
  extern uint32 ro_data_end_address;
  extern uint32 apstartup_text_load_begin;
  extern uint32 apstartup_text_load_end;

  // Map kernel pages
  for(VAddr a{ArchCommon::getKernelStartAddress()}; a.addr < ArchCommon::getKernelEndAddress(); a.addr += PAGE_SIZE)
  {
      size_t pti = (a.pdpti - k_start.pdpti)*PAGE_DIRECTORY_ENTRIES*PAGE_DIRECTORY_POINTER_TABLE_ENTRIES + (a.pdi - k_start.pdi)*PAGE_TABLE_ENTRIES + a.pti;
      assert(pti < sizeof(kernel_page_tables)/sizeof(kernel_page_tables[0]));
      pte_start[pti].page_ppn = VIRTUAL_TO_PHYSICAL_BOOT(a.addr)/PAGE_SIZE;
      // AP startup pages need to be writeable to fill in the GDT, ...
      size_t ap_text_start = ((size_t)&apstartup_text_load_begin/PAGE_SIZE)*PAGE_SIZE;
      size_t ap_text_end = (size_t)&apstartup_text_load_end;

      pte_start[pti].writeable =
          (((a.addr >= (pointer)&text_start_address) &&
            (a.addr < (pointer)&ro_data_end_address)) &&
           !((a.addr >= ap_text_start) &&
             (a.addr < ap_text_end))
           ? 0
           : 1);
      pte_start[pti].present = 1;
  }

  if (ArchCommon::haveVESAConsole(0))
  {
    for (i = 0; i < 8; ++i)
    {
      pde_start[1528 + i].page.present = 1;
      pde_start[1528 + i].page.writeable = 1;
      pde_start[1528 + i].page.size = 1;
      pde_start[1528 + i].page.cache_disabled = 1;
      pde_start[1528 + i].page.write_through = 1;
      pde_start[1528 + i].page.page_ppn = (ArchCommon::getVESAConsoleLFBPtr(0) / (1024 * 1024 * 2)) + i;
    }
  }

  // identity mapping
  for (i = 0; i < PAGE_DIRECTORY_ENTRIES; ++i)
  {
    pde_start[i + PAGE_DIRECTORY_ENTRIES * 3].page.present = 1;
    pde_start[i + PAGE_DIRECTORY_ENTRIES * 3].page.writeable = 1;
    pde_start[i + PAGE_DIRECTORY_ENTRIES * 3].page.size = 1;
    pde_start[i + PAGE_DIRECTORY_ENTRIES * 3].page.page_ppn = i;
  }
}

extern "C" void removeBootTimeIdentMapping()
{
  uint32 i;

  kernel_page_directory_pointer_table[0].present = 0;
  kernel_page_directory_pointer_table[1].present = 0;
  for (i = 0; i < 10; ++i)
  {
    kernel_page_directory[i].page.present = 0;
  }
}
