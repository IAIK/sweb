/**
 * $Id: init_boottime_pagetables.cpp,v 1.11 2005/09/03 18:20:14 nomenquis Exp $
 *
 * $Log: init_boottime_pagetables.cpp,v $
 * Revision 1.9  2005/04/27 08:58:16  nomenquis
 * locks work!
 * w00t !
 *
 * Revision 1.8  2005/04/26 17:03:27  nomenquis
 * 16 bit framebuffer hack
 *
 * Revision 1.7  2005/04/26 10:58:14  nomenquis
 * and now it really works
 *
 * Revision 1.6  2005/04/26 10:23:54  nomenquis
 * kernel at 2gig again, not 2gig + 1m since were not using 4m pages anymore
 *
 * Revision 1.5  2005/04/25 23:23:48  btittelbach
 * nothing really
 *
 * Revision 1.4  2005/04/25 23:09:18  nomenquis
 * fubar 2
 *
 * Revision 1.3  2005/04/25 22:41:58  nomenquis
 * foobar
 *
 * Revision 1.2  2005/04/22 18:23:04  nomenquis
 * massive cleanups
 *
 * Revision 1.1  2005/04/22 17:40:58  nomenquis
 * cleanup
 *
 * Revision 1.7  2005/04/21 21:31:24  nomenquis
 * added lfb support, also we now use a different grub version
 * we also now read in the grub multiboot version
 *
 * Revision 1.6  2005/04/20 20:42:56  nomenquis
 * refined debuggability for bootstrapping
 * also using 4m mapping for 3g ident and 4k mapping with 4 ptes (but only one used) for 2g
 *
 * Revision 1.5  2005/04/20 15:26:35  nomenquis
 * more and more stuff actually works
 *
 * Revision 1.4  2005/04/20 09:00:29  nomenquis
 * removed lots of trash symbols, and added "nice" symbol names
 * also added docs about these symbols into the wiki
 *
 * Revision 1.3  2005/04/20 07:09:59  nomenquis
 * added inital paging for the kernel, plus mapping to two gigs
 * hoever, the kernel now is at 1meg phys and 2gig + 1 meg virtual due to these 4m pages
 *
 * Revision 1.2  2005/04/12 18:42:50  nomenquis
 * changed a zillion of iles
 *
 * Revision 1.1  2005/04/12 17:46:44  nomenquis
 * added lots of files
 *
 *
 */

#include "types.h"
#include "boot-time.h"
#include "paging-definitions.h"
#include "offsets.h"
#include "multiboot.h"
#include "ArchCommon.h"

extern page_directory_entry kernel_page_directory_start[];
extern page_table_entry kernel_page_tables_start[];
extern void* kernel_end_address;

extern "C" void initialiseBootTimePaging();
#define print(x)     fb_start += 2; \
    { \
      uint32 divisor; \
      uint32 current; \
      uint32 remainder; \
      current = (uint32)x; \
      divisor = 1000000000; \
      while (divisor > 0) \
      { \
        remainder = current % divisor; \
        current = current / divisor; \
        \
        fb[fb_start++] = (uint8)current + '0' ; \
        fb[fb_start++] = 0x9f ; \
    \
        divisor = divisor / 10; \
        current = remainder; \
      }      \
    }
  

uint32 fb_start_hack = 0;

uint32 isPageUsed(uint32 page_number)
{

   uint32 &fb_start = *((uint32*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&fb_start_hack));
   uint8 * fb = (uint8*) 0x000B8000;
   uint32 i;
   uint32 num_modules = ArchCommon::getNumModules(0);
   for (i=0;i<num_modules;++i)
   {
      uint32 start_page = ArchCommon::getModuleStartAddress(i,0) / PAGE_SIZE;
      uint32 end_page = ArchCommon::getModuleEndAddress(i,0) / PAGE_SIZE;
      
      if ( start_page <= page_number && end_page >= page_number)
      {
         print(page_number);

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
  // well, the concerned reader might ask himself "why the fuck does this work?"
  // the answer is simple, it does work because the beloved compiler generates 
  // relative calls in this case.
  // if the compiler would generate an absolut call we'd be screwed since we 
  // have not set up paging yet :)
void initialiseBootTimePaging()
{
  uint32 i;

  uint8 * fb = (uint8*) 0x000B8000;
  uint32 &fb_start = *((uint32*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&fb_start_hack));
  page_directory_entry *pde_start = (page_directory_entry*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&kernel_page_directory_start);
  //uint8 *pde_start_bytes = (uint8 *)pde_start;
  page_table_entry *pte_start = (page_table_entry*)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&kernel_page_tables_start);
  
  uint32 kernel_last_page = ((uint32)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&kernel_end_address)) / PAGE_SIZE;
  uint32 first_free_page = kernel_last_page + 1;
   
  print((uint32)VIRTUAL_TO_PHYSICAL_BOOT((pointer)&kernel_end_address));
  print(first_free_page);

 
  // we do not have to clear the pde since its in the bss  
  for (i=0;i<5;++i)
  {
    pde_start[i].pde4m.present = 1;
    pde_start[i].pde4m.writeable = 1;
    pde_start[i].pde4m.use_4_m_pages = 1;
    pde_start[i].pde4m.page_base_address = i;
  }

  for (i=0;i<4;++i)
  {
    pde_start[i+512].pde4k.present = 1;
    pde_start[i+512].pde4k.writeable = 1;
    pde_start[i+512].pde4k.page_table_base_address = ((pointer)&pte_start[1024*i])/PAGE_SIZE;
  }
 
  // ok, we currently only fill in mappings for the first 4 megs (aka one page table)
  // we do not have to zero out the other page tables since they're alreay empty
  // thanks to the bss clearance.
  
  // update, from now on, all pages up to the last page containing only rodata
  // will be write protected.
  
  // DAMN IT, THIS TOTALLY SUCKS, I got it wrong like 3 times!
  // AAAAAAAAAAAAAAAAAH, now the 4th time
  
  extern uint32 ro_data_end_address;
  pointer rod = (pointer)&ro_data_end_address;
  
  uint32 last_ro_data_page = (rod-LINK_BASE)/PAGE_SIZE;

//  last_ro_data_page = 0;
  for (i=0;i<last_ro_data_page;++i)
  {
    pte_start[i].present = 1;
    pte_start[i].writeable = 0;
    pte_start[i].page_base_address = i+256;
  }
  print(first_free_page);
  
  for (i=last_ro_data_page;i<(first_free_page-256);++i)
  {
    pte_start[i].present = 1;
    pte_start[i].writeable = 1;
    pte_start[i].page_base_address = i+256;
  }
  uint32 start_page = first_free_page;
  
  for (i=first_free_page-256;i<1024;++i)
  {
    pte_start[i].present = 1;
    pte_start[i].writeable = 1;
    pte_start[i].page_base_address = getNextFreePage(start_page);
    start_page = pte_start[i].page_base_address+1;
    
  }
  

  if (ArchCommon::haveVESAConsole(0))
  {
    for (i=0;i<4;++i)
    {
      pde_start[764+i].pde4m.present = 1;
      pde_start[764+i].pde4m.writeable = 1;
      pde_start[764+i].pde4m.use_4_m_pages = 1;
      pde_start[764+i].pde4m.cache_disabled = 1;
      pde_start[764+i].pde4m.write_through = 1;
      pde_start[764+i].pde4m.page_base_address = (ArchCommon::getVESAConsoleLFBPtr(0) / (1024*1024*4))+i;
    }     
  }

  for (i=0;i<256;++i)
  {
    pde_start[i+768].pde4m.present = 1;
    pde_start[i+768].pde4m.writeable = 1;
    pde_start[i+768].pde4m.use_4_m_pages = 1;
    pde_start[i+768].pde4m.page_base_address = i;
  }

}

void removeBootTimeIdentMapping()
{
  uint32 i;

  for (i=0;i<5;++i)
  {
    kernel_page_directory_start[i].pde4m.present=0;
  }
}
