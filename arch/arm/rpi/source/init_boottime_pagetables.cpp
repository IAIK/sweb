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

extern page_directory_entry kernel_page_directory_start[];
extern void* kernel_end_address;
extern void* interrupt_vector_table;
extern char* currentStack;

extern "C" void initialiseBootTimePaging();
extern "C" void removeBootTimeIdentMapping();

#define TO_HEX(X,Y) (((((X) >> Y) & 0xF) < 10) ? (((X) >> Y) & 0xF) + '0' : (((X) >> Y) & 0xF) - 0xA + 'A')
#define PRINT_ADDRESS(X) do { \
kprintfd("%c",TO_HEX((uint32)X,28)); \
kprintfd("%c",TO_HEX((uint32)X,24)); \
kprintfd("%c",TO_HEX((uint32)X,20)); \
kprintfd("%c",TO_HEX((uint32)X,16)); \
kprintfd("%c",TO_HEX((uint32)X,12)); \
kprintfd("%c",TO_HEX((uint32)X,8)); \
kprintfd("%c",TO_HEX((uint32)X,4)); \
kprintfd("%c\n",TO_HEX((uint32)X,0)); \
} while (0)

static void mapPage(page_directory_entry *pde_start, uint32 pdi, uint32 ppn_1m)
{
  pde_start[pdi].pde1m.reserved_1 = 0;
  pde_start[pdi].pde1m.permissions = 1;
  pde_start[pdi].pde1m.reserved_2 = 0;
  pde_start[pdi].pde1m.domain = 0;
  pde_start[pdi].pde1m.reserved_3 = 0;
  pde_start[pdi].pde1m.cachable = 0;
  pde_start[pdi].pde1m.bufferable = 0;
  pde_start[pdi].pde1m.size = 2;
  pde_start[pdi].pde1m.page_ppn = ppn_1m;
}

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
  mapPage(pde_start,0x900,0x200);  // gpu
  for (i = 0; i < 8; ++i)
    mapPage(pde_start,0xb00 + i,0x5c0 +i);  // framebuffer
  // still plenty of room for more memory mapped devices
}

void removeBootTimeIdentMapping()
{
  // we will not remove anything because we need the first 8 mb 1:1 mapped
}
