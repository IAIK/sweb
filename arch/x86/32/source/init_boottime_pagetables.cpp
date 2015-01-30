/**
 * @file init_boottime_pagetables.cpp
 *
 */

#include "types.h"
#include "paging-definitions.h"
#include "offsets.h"
#include "multiboot.h"
#include "ArchCommon.h"
#include "debug_bochs.h"
#include "ArchMemory.h"

extern void* kernel_end_address;

uint32 isPageUsed(uint32 page_number)
{
   uint32 i;
   uint32 num_modules = ArchCommon::getNumModules(0);
   for (i=0;i<num_modules;++i)
   {
      uint32 start_page = ArchCommon::getModuleStartAddress(i,0) / PAGE_SIZE;
      uint32 end_page = ArchCommon::getModuleEndAddress(i,0) / PAGE_SIZE;

      if ( start_page <= page_number && end_page >= page_number)
      {
         return 1;
      }
   }

   return 0;
}

uint32 getNextFreePage(uint32 page_number)
{
   while(isPageUsed(page_number))
     page_number++;
   return page_number;
}

extern "C" void initialiseBootTimePaging()
{
  uint32 i;

  PageDirEntry *pde_start = (PageDirEntry*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)kernel_page_directory);

  PageTableEntry *pte_start = (PageTableEntry*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)kernel_page_tables);

  uint32 kernel_last_page = ((uint32)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&kernel_end_address)) / PAGE_SIZE;
  uint32 first_free_page = kernel_last_page + 1;

  // we do not have to clear the pde since its in the bss
  for (i = 0; i < 5; ++i)
  {
    pde_start[i].page.present = 1;
    pde_start[i].page.writeable = 1;
    pde_start[i].page.size = 1;
    pde_start[i].page.page_ppn = i;
  }

  for (i=0;i<4;++i)
  {
    pde_start[i+512].pt.present = 1;
    pde_start[i+512].pt.writeable = 1;
    pde_start[i+512].pt.page_table_ppn = ((pointer)&pte_start[1024*i])/PAGE_SIZE;
  }

  // ok, we currently only fill in mappings for the first 4 megs (aka one page table)
  // we do not have to zero out the other page tables since they're already empty
  // thanks to the bss clearance.

  // update, from now on, all pages up to the last page containing only rodata
  // will be write protected.

  extern uint32 ro_data_end_address;
  pointer rod = (pointer)&ro_data_end_address;

  uint32 last_ro_data_page = (rod-LINK_BASE)/PAGE_SIZE;

//  last_ro_data_page = 0;
  for (i=0;i<last_ro_data_page;++i)
  {
    pte_start[i].present = 1;
    pte_start[i].writeable = 0;
    pte_start[i].page_ppn = i+256;
  }
  for (i=last_ro_data_page;i<(first_free_page-256);++i)
  {
    pte_start[i].present = 1;
    pte_start[i].writeable = 1;
    pte_start[i].page_ppn = i+256;
  }
  uint32 start_page = first_free_page;

  //HACK Example: Extend Kernel Memory (by Bernhard T.):
  // just change 1024 here to anything <= 3*1024  (because we have 3 pte's in bss which were mapped in the pde above)
  for (i=first_free_page-256;i<1024;++i)
  {
    pte_start[i].present = 1;
    pte_start[i].writeable = 1;
    pte_start[i].page_ppn = getNextFreePage(start_page);
    start_page = pte_start[i].page_ppn+1;
  }

  if (ArchCommon::haveVESAConsole(0))
  {
    for (i=0;i<4;++i)
    {
      pde_start[764+i].page.present = 1;
      pde_start[764+i].page.writeable = 1;
      pde_start[764+i].page.size = 1;
      pde_start[764+i].page.cache_disabled = 1;
      pde_start[764+i].page.write_through = 1;
      pde_start[764+i].page.page_ppn = (ArchCommon::getVESAConsoleLFBPtr(0) / (1024*1024*4))+i;
    }
  }

  for (i=0;i<256;++i)
  {
    pde_start[i+768].page.present = 1;
    pde_start[i+768].page.writeable = 1;
    pde_start[i+768].page.size = 1;
    pde_start[i+768].page.page_ppn = i;
  }
}

extern "C" void removeBootTimeIdentMapping()
{
  uint32 i;

  for (i=0;i<5;++i)
  {
    kernel_page_directory[i].page.present=0;
  }
}
