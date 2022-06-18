#include "types.h"
#include "paging-definitions.h"
#include "offsets.h"
#include "multiboot.h"
#include "ArchCommon.h"
#include "debug_bochs.h"
#include "ArchMemory.h"
#include "assert.h"

extern "C" void initialiseBootTimePaging()
{
  uint32 i;

  PageDirEntry *pde_start = (PageDirEntry*) VIRTUAL_TO_PHYSICAL_BOOT((pointer )kernel_page_directory);
  PageTableEntry *pte_start = (PageTableEntry*) VIRTUAL_TO_PHYSICAL_BOOT((pointer )kernel_page_tables);


  VAddr k_start{ArchCommon::getKernelStartAddress()};
  VAddr k_end{ArchCommon::getKernelEndAddress()};

  // Boot time ident mapping
  for (i = 0; i < 5; ++i)
  {
    pde_start[i].page.page_ppn = i;
    pde_start[i].page.writeable = 1;
    pde_start[i].page.size = 1;
    pde_start[i].page.present = 1;
  }



  // Map kernel page tables
  for (size_t pdi = k_start.pdi; pdi <= k_end.pdi; ++pdi)
  {
    size_t pdi_offset = pdi - k_start.pdi;
    pde_start[pdi].pt.page_table_ppn = ((pointer) &pte_start[PAGE_TABLE_ENTRIES * pdi_offset]) / PAGE_SIZE;
    pde_start[pdi].pt.writeable = 1;
    pde_start[pdi].pt.present = 1;
  }

  extern uint32 text_start_address;
  extern uint32 ro_data_end_address;
  extern uint32 apstartup_text_load_begin;
  extern uint32 apstartup_text_load_end;

  // Map kernel page tables
  for(VAddr a{ArchCommon::getKernelStartAddress()}; a.addr < ArchCommon::getKernelEndAddress(); a.addr += PAGE_SIZE)
  {
    size_t pti = (a.pdi - k_start.pdi)*PAGE_TABLE_ENTRIES + a.pti;
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
    for (i = 0; i < 4; ++i)
    {
      pde_start[764 + i].page.present = 1;
      pde_start[764 + i].page.writeable = 1;
      pde_start[764 + i].page.size = 1;
      pde_start[764 + i].page.cache_disabled = 1;
      pde_start[764 + i].page.write_through = 1;
      pde_start[764 + i].page.page_ppn = (ArchCommon::getVESAConsoleLFBPtr(0) / (1024 * 1024 * 4)) + i;
    }
  }

  // identity mapping
  for (i = 0; i < 1024-768; ++i)
  {
    pde_start[i + 768].page.present = 1;
    pde_start[i + 768].page.writeable = 1;
    pde_start[i + 768].page.size = 1;
    pde_start[i + 768].page.page_ppn = i;
  }
}

extern "C" void removeBootTimeIdentMapping()
{
  uint32 i;

  for (i = 0; i < 5; ++i)
  {
    kernel_page_directory[i].page.present = 0;
  }
}
