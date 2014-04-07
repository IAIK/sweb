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

void initialiseBootTimePaging()
{
  page_directory_entry *pde_start = (page_directory_entry*)(((void*)kernel_page_directory_start) - PHYSICAL_TO_VIRTUAL_OFFSET);

  uint32 i;
  for (i = 0; i < 4096; ++i)
  {
    if (i >= 2048 && i <= 2048+4) // kernel
    {
      pde_start[i].pde1m.base = i-2048;
    }
    else if (i >= 3328) // identity mapping starting from 0xD0000000
    {
      pde_start[i].pde1m.base = i-3328;
    }
    else
    {
      pde_start[i].pde1m.base = i;
    }
    pde_start[i].pde1m.reserved_1 = 0;
    pde_start[i].pde1m.permissions = 3;
    pde_start[i].pde1m.reserved_2 = 0;
    pde_start[i].pde1m.domain = 0;
    pde_start[i].pde1m.reserved_3 = 0;
    pde_start[i].pde1m.cachable = 0;
    pde_start[i].pde1m.bufferable = 0;
    pde_start[i].pde1m.size = 2;
  }
}

void removeBootTimeIdentMapping()
{
//  uint32 i;
//
//  for (i=0;i<5;++i)
//  {
//    kernel_page_directory_start[i].pde4m.present=0;
//  }
}
