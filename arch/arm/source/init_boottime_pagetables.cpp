/**
 * @file init_boottime_pagetables.cpp
 *
 */

#include "types.h"
#include "boot-time.h"
#include "paging-definitions.h"
#include "offsets.h"
#include "multiboot.h"
#include "ArchCommon.h"
#include "kprintf.h"

extern page_directory_entry kernel_page_directory_start[];
extern void* kernel_end_address;

extern "C" void initialiseBootTimePaging();
extern "C" void removeBootTimeIdentMapping();

uint32 fb_start_hack = 0;

uint32 isPageUsed(uint32 page_number)
{
   uint32 &fb_start = *((uint32*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&fb_start_hack));
   uint8 *fb = (uint8*) 0x000B8000;
   uint32 i;
   uint32 num_modules = ArchCommon::getNumModules(0);
   for (i=0;i<num_modules;++i)
   {
      uint32 start_page = ArchCommon::getModuleStartAddress(i,0) / PAGE_SIZE;
      uint32 end_page = ArchCommon::getModuleEndAddress(i,0) / PAGE_SIZE;

      if ( start_page <= page_number && end_page >= page_number)
      {
         //print(page_number);

         return 1;
      }
   }

   return 0;
}

uint32 getNextFreePage(uint32 page_number)
{
   while(isPageUsed(page_number))
     page_number++;
   return page_number;
}

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

// Be careful, this works because the beloved compiler generates
// relative calls in this case.
// if the compiler generated an absolut call we'd be screwed since we
// have not set up paging yet :)
void initialiseBootTimePaging()
{
  kprintfd("initialiseBootTimePaging: start\n");
  PRINT_ADDRESS(&initialiseBootTimePaging);
  PRINT_ADDRESS(&kernel_page_directory_start);
  uint32 i;

  PRINT_ADDRESS(PHYSICAL_TO_VIRTUAL_OFFSET);
  page_directory_entry *pde_start = (page_directory_entry*)(((void*)kernel_page_directory_start) - PHYSICAL_TO_VIRTUAL_OFFSET);
  PRINT_ADDRESS(pde_start);

  // we do not have to clear the pde since its in the bss
  for (i = 0; i < 2048; ++i)
  {
    pde_start[i].pde1m.base = i;
    pde_start[i].pde1m.reserved_1 = 0;
    pde_start[i].pde1m.permissions = 3;
    pde_start[i].pde1m.reserved_2 = 0;
    pde_start[i].pde1m.domain = 0;
    pde_start[i].pde1m.reserved_3 = 0;
    pde_start[i].pde1m.cachable = 0;
    pde_start[i].pde1m.bufferable = 0;
    pde_start[i].pde1m.size = 2;
    pde_start[i+2048].pde1m.base = i;
    pde_start[i+2048].pde1m.reserved_1 = 0;
    pde_start[i+2048].pde1m.permissions = 3;
    pde_start[i+2048].pde1m.reserved_2 = 0;
    pde_start[i+2048].pde1m.domain = 0;
    pde_start[i+2048].pde1m.reserved_3 = 0;
    pde_start[i+2048].pde1m.cachable = 0;
    pde_start[i+2048].pde1m.bufferable = 0;
    pde_start[i+2048].pde1m.size = 2;
    PRINT_ADDRESS(pde_start + i);
    PRINT_ADDRESS(pde_start + i + 2048);
    PRINT_ADDRESS(*(uint32*)(pde_start + i));
  }
  kprintfd("initialiseBootTimePaging: done\n");

}

void removeBootTimeIdentMapping()
{

  for (;;)
    kprintfd("here\n");
  uint32 i;
//
//  for (i=0;i<5;++i)
//  {
//    kernel_page_directory_start[i].pde4m.present=0;
//  }
}
