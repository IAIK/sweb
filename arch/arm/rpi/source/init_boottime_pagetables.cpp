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
  mapPage(pde_start,0x860,0x202);  // pl011
  mapPage(pde_start,0x8C0,0x203);  // emmc
  mapPage(pde_start,0x900,0x200);  // most devices (ic, timer, gpu, ...)

  mapPage(pde_start,0x909,0x209);  // map for csud

  for (i = 0; i < 8; ++i)
    mapPage(pde_start,0xb00 + i,0x5c0 +i);  // framebuffer TODO: this might be not necessary
  // still plenty of room for more memory mapped devices
}

void removeBootTimeIdentMapping()
{
  // we will not remove anything because we need the first 8 mb 1:1 mapped
}
