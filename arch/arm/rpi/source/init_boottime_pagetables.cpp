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
  mapBootTimePage(pde_start,0x860,0x202);  // pl011
  mapBootTimePage(pde_start,0x8C0,0x203);  // emmc
  mapBootTimePage(pde_start,0x900,0x200);  // most devices (ic, timer, gpu, ...)

  mapBootTimePage(pde_start,0x909,0x209);  // map for csud

  for (i = 0; i < 8; ++i)
    mapBootTimePage(pde_start,0xb00 + i,0x5c0 +i);  // framebuffer TODO: this might be not necessary
  // still plenty of room for more memory mapped devices
}

extern "C" void removeBootTimeIdentMapping()
{
  // we will not remove anything because we need the first 8 mb 1:1 mapped
}
