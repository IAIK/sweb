#include "init_boottime_pagetables.h"

#include "offsets.h"
#include "paging-definitions.h"

#include "types.h"

extern "C" void initialiseBootTimePaging()
{
  PageDirEntry *pde_start = (PageDirEntry*) VIRTUAL_TO_PHYSICAL_BOOT((pointer )kernel_page_directory);
  PageTableEntry *pte_start = (PageTableEntry*) VIRTUAL_TO_PHYSICAL_BOOT((pointer )kernel_page_tables);

  uint32 i;
  // clear the page dir
  for (i = 0; i < 4096; ++i)
    *((uint32*)pde_start + i) = 0;
  // 1 : 1 mapping of the first 8 mbs
  for (i = 0; i < 8; ++i)
    mapBootTimePage(pde_start, i, i);
  // map 16 mb PTs for kernel
  for (i = 0; i < 16; ++i)
  {
    pde_start[2048 + i].pt.size = 1;
    pde_start[2048 + i].pt.offset = i % 4;
    pde_start[2048 + i].pt.pt_ppn = ((pointer) &pte_start[PAGE_TABLE_ENTRIES * i]) / PAGE_SIZE;
  }
  // clear the page tables
  for (i = 0; i < 16 * PAGE_TABLE_ENTRIES; ++i)
    *((uint32*)pte_start + i) = 0;
  // map kernel into PTs
  size_t kernel_last_page = (size_t)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&kernel_end_address) / PAGE_SIZE;
  extern size_t ro_data_end_address;
  size_t last_ro_data_page = (size_t)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&ro_data_end_address) / PAGE_SIZE;

  for (i = 0; i < last_ro_data_page; ++i)
  {
    pte_start[i].size = 2;
    pte_start[i].permissions = 1;
    pte_start[i].page_ppn = i;
  }
  for (; i < kernel_last_page; ++i)
  {
    pte_start[i].size = 2;
    pte_start[i].permissions = 1;
    pte_start[i].page_ppn = i;
  }
  // 3gb 1:1 mapping
  for (i = 0; i < 1024; ++i)
    mapBootTimePage(pde_start, 3072 + i, i);
  // map devices from 0x81000000 upwards
  for (i = 0; i < 8; ++i)
    mapBootTimePage(pde_start, 0x810 + i,0x100 + i); // integratorcm 8M
  mapBootTimePage(pde_start,0x830,0x130);  // icp_pit
  for (i = 0; i < 8; ++i)
    mapBootTimePage(pde_start, 0x840 + i,0x140 + i); // icp-pic 8M
  mapBootTimePage(pde_start,0x850,0x150);  // pl031
  mapBootTimePage(pde_start,0x860,0x160);  // pl011
  mapBootTimePage(pde_start,0x870,0x170);  // pl011
  mapBootTimePage(pde_start,0x880,0x180);  // pl050
  mapBootTimePage(pde_start,0x890,0x190);  // pl050
  mapBootTimePage(pde_start,0x8c0,0x1c0);  // pl181
  mapBootTimePage(pde_start,0x900,0xc00);  // pl110
  mapBootTimePage(pde_start,0x980,0xc80);  // smc91c111-mmio
  for (i = 0; i < 8; ++i)
    mapBootTimePage(pde_start,0x9a0 + i,0xca0 + i);  // icp-pic
  for (i = 0; i < 8; ++i)
    mapBootTimePage(pde_start,0x9b0 + i,0xcb0 + i);  // control
  // still plenty of room for more memory mapped devices
}

extern "C" void removeBootTimeIdentMapping()
{
  // we could remove parts of the mapping but we need the interrupt vector at 0x0
}
