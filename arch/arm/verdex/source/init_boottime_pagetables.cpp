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
#include "debug_bochs.h"

extern page_directory_entry kernel_page_directory_start[];
extern void* kernel_end_address;
extern void* interrupt_vector_table;
extern char* currentStack;

extern "C" void initialiseBootTimePaging();
extern "C" void removeBootTimeIdentMapping();

#define TO_HEX(X,Y) ((((((uint32)X) >> Y) & 0xF) < 10) ? ((((uint32)X) >> Y) & 0xF) + '0' : ((((uint32)X) >> Y) & 0xF) - 0xA + 'A')
#define PRINT_ADDRESS(X) do { \
kprintfd("%c",TO_HEX(X,28)); \
kprintfd("%c",TO_HEX(X,24)); \
kprintfd("%c",TO_HEX(X,20)); \
kprintfd("%c",TO_HEX(X,16)); \
kprintfd("%c",TO_HEX(X,12)); \
kprintfd("%c",TO_HEX(X,8)); \
kprintfd("%c",TO_HEX(X,4)); \
kprintfd("%c\n",TO_HEX(X,0)); \
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
  *(volatile unsigned long*)(0x40100000) = TO_HEX(&writeChar2Bochs,28);
  *(volatile unsigned long*)(0x40100000) = TO_HEX(&writeChar2Bochs,24);
  *(volatile unsigned long*)(0x40100000) = TO_HEX(&writeChar2Bochs,20);
  *(volatile unsigned long*)(0x40100000) = TO_HEX(&writeChar2Bochs,16);
  *(volatile unsigned long*)(0x40100000) = TO_HEX(&writeChar2Bochs,12);
  *(volatile unsigned long*)(0x40100000) = TO_HEX(&writeChar2Bochs,8);
  *(volatile unsigned long*)(0x40100000) = TO_HEX(&writeChar2Bochs,4);
  *(volatile unsigned long*)(0x40100000) = TO_HEX(&writeChar2Bochs,0);
  *(volatile unsigned long*)(0x40100000) = '\n';

  uint32 i;
  // the verdex board has physical ram mapped to 0xA0000000
  uint32 base = 0xA00;
  for (i = 0; i < 4096; ++i)
      mapPage(pde_start, i, i);
  // clear the page dir
  for (i = 0; i < 4096; ++i)
    *((uint32*)pde_start) = 0;
  // 1 : 1 mapping of the first 8 mbs
  for (i = 0; i < 8; ++i)
    mapPage(pde_start, i, i);
  // 1 : 1 mapping of the first 8 mbs of physical ram
  for (i = 0; i < 8; ++i)
    mapPage(pde_start, base + i, base + i);
  // map first 4 mb for kernel
  for (i = 0; i < 4; ++i)
    mapPage(pde_start, 0x800 + i, base + i);
  // 3gb 1:1 mapping
  for (i = 0; i < 1024; ++i)
    mapPage(pde_start, 0xC00 + i, base + i);
  // map devices from 0x81000000 upwards
  mapPage(pde_start,0x860,0x401);  // uart device
}

void removeBootTimeIdentMapping()
{
  *(volatile unsigned long*)(0x86000000) = 'R';
  // we will not remove anything because we need the first 8 mb 1:1 mapped
}
