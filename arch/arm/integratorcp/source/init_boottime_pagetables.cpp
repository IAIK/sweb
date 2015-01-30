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
  PageDirEntry *pde_start = (PageDirEntry*)(((char*)kernel_page_directory) - PHYSICAL_TO_VIRTUAL_OFFSET);

  uint32 i;
  // clear the page dir
  for (i = 0; i < 4096; ++i)
    *((uint32*)pde_start) = 0;
  // 1 : 1 mapping of the first 8 mbs
  for (i = 0; i < 8; ++i)
    mapBootTimePage(pde_start, i, i);
  // map first 4 mb for kernel
  for (i = 0; i < 4; ++i)
    mapBootTimePage(pde_start, 2048 + i, i);
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
  // we will not remove anything because we need the first 8 mb 1:1 mapped
}
