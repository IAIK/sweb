/**
 * @file init_boottime_pagetables.cpp
 *
 */

#include "types.h"
#include "boot-time.h"
#include "paging-definitions.h"
#include "offsets.h"
#include "ArchCommon.h"
#include "kprintf.h"
#include "init_boottime_pagetables.h"

void initialiseBootTimePaging()
{
  page_directory_entry *pde_start = (page_directory_entry*)(((void*)kernel_page_directory_start) - PHYSICAL_TO_VIRTUAL_OFFSET);

  uint32 i;
  // clear the page dir
  for (i = 0; i < 4096; ++i)
    *((uint32*)pde_start) = 0;
  // 1 : 1 mapping of the first 8 mbs
  for (i = 0; i < 8; ++i)
    mapPage(pde_start, i, i);
  // map first 4 mb for kernel
  for (i = 0; i < 4; ++i)
    mapPage(pde_start, 2048 + i, i);
  // 3gb 1:1 mapping
  for (i = 0; i < 1024; ++i)
    mapPage(pde_start, 3072 + i, i);
  // map devices from 0x81000000 upwards
  for (i = 0; i < 8; ++i)
    mapPage(pde_start, 0x810 + i,0x100 + i); // integratorcm 8M
  mapPage(pde_start,0x830,0x130);  // icp_pit
  for (i = 0; i < 8; ++i)
    mapPage(pde_start, 0x840 + i,0x140 + i); // icp-pic 8M
  mapPage(pde_start,0x850,0x150);  // pl031
  mapPage(pde_start,0x860,0x160);  // pl011
  mapPage(pde_start,0x870,0x170);  // pl011
  mapPage(pde_start,0x880,0x180);  // pl050
  mapPage(pde_start,0x890,0x190);  // pl050
  mapPage(pde_start,0x8c0,0x1c0);  // pl181
  mapPage(pde_start,0x900,0xc00);  // pl110
  mapPage(pde_start,0x980,0xc80);  // smc91c111-mmio
  for (i = 0; i < 8; ++i)
    mapPage(pde_start,0x9a0 + i,0xca0 + i);  // icp-pic
  for (i = 0; i < 8; ++i)
    mapPage(pde_start,0x9b0 + i,0xcb0 + i);  // control
  // still plenty of room for more memory mapped devices
}

void removeBootTimeIdentMapping()
{
  // we will not remove anything because we need the first 8 mb 1:1 mapped
}
