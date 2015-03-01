/**
 * @file init_boottime_pagetables.cpp
 *
 */

#include "types.h"
#include "paging-definitions.h"
#include "offsets.h"
#include "init_boottime_pagetables.h"

extern "C" void initialiseBootTimePaging()
{
  PageDirEntry *pde_start = (PageDirEntry*) VIRTUAL_TO_PHYSICAL_BOOT((pointer)kernel_page_directory);
  PageTableEntry *pte_start = (PageTableEntry*) VIRTUAL_TO_PHYSICAL_BOOT((pointer)kernel_page_tables);

  uint32 i;
  // the verdex board has physical ram mapped to 0xA0000000
  uint32 base = 0xA00;
  uint32 base_4k = 0xA0000;
  // clear the page dir
  for (i = 0; i < 4096; ++i)
    *((uint32*)pde_start + i) = 0;
  // map 16 mb PTs for kernel
  for (i = 0; i < 16; ++i)
  {
    pde_start[2048 + i].pt.size = 1;
    pde_start[2048 + i].pt.offset = i % 4;
    pde_start[2048 + i].pt.pt_ppn = (((pointer) &pte_start[PAGE_TABLE_ENTRIES * i]) / PAGE_SIZE);
  }
  // clear the page tables
  for (i = 0; i < 16 * PAGE_TABLE_ENTRIES; ++i)
    *((uint32*)pte_start + i) = 0;
  // map kernel into PTs
  size_t kernel_last_page = (size_t)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&kernel_end_address) / PAGE_SIZE;
  extern size_t ro_data_end_address;
  size_t last_ro_data_page = (size_t)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&ro_data_end_address) / PAGE_SIZE;
  for (i = 0; i < last_ro_data_page - base_4k; ++i)
  {
    pte_start[i].size = 2;
    pte_start[i].permissions = 1;
    pte_start[i].page_ppn = i + base_4k;
  }
  for (; i < kernel_last_page - base_4k; ++i)
  {
    pte_start[i].size = 2;
    pte_start[i].permissions = 1;
    pte_start[i].page_ppn = i + base_4k;
  }
  // 1 : 1 mapping of the first 8 mbs
  for (i = 0; i < 8; ++i)
    mapBootTimePage(pde_start, i, base + i);
  // 1 : 1 mapping of the first 8 mbs of physical ram
  for (i = 0; i < 8; ++i)
    mapBootTimePage(pde_start, base + i, base + i);
  // map first 4 mb for kernel TODO: remove this loop!
  for (i = 0; i < 4; ++i)
    mapBootTimePage(pde_start, 0x800 + i, base + i);
  // 3gb 1:1 mapping
  for (i = 0; i < 1024; ++i)
    mapBootTimePage(pde_start, 0xC00 + i, base + i);
  // map devices from 0x81000000 upwards
  mapBootTimePage(pde_start,0x860,0x401);  // uart device
  mapBootTimePage(pde_start,0x900,0x440);  // lcd controller
  mapBootTimePage(pde_start,0x840,0x40D);  // interrupt controller
  mapBootTimePage(pde_start,0x830,0x40A);  // timer
  mapBootTimePage(pde_start,0x8C0,0x411);  // mmc controller
}

extern "C" void removeBootTimeIdentMapping()
{
  // we will not remove anything because we need the first 8 mb 1:1 mapped
}
