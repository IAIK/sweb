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
  // the verdex board has physical ram mapped to 0xA0000000
  uint32 base = 0xA00;
  // clear the page dir
  for (i = 0; i < 4096; ++i)
    *((uint32*)pde_start) = 0;
  // 1 : 1 mapping of the first 8 mbs
  for (i = 0; i < 8; ++i)
    mapBootTimePage(pde_start, i, base + i);
  // 1 : 1 mapping of the first 8 mbs of physical ram
  for (i = 0; i < 8; ++i)
    mapBootTimePage(pde_start, base + i, base + i);
  // map first 4 mb for kernel
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
