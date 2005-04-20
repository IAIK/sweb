/**
 * $Id: init_boottime_pagetables.c,v 1.4 2005/04/20 09:00:29 nomenquis Exp $
 *
 * $Log: init_boottime_pagetables.c,v $
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


#define BOOT_TIME_PAGE_DIRECTORY_START (1024*1024*4)


void initialiseBootTimePaging()
{
  uint32 i,k;
 
  page_directory_entry *pde_start = (page_directory_entry*)BOOT_TIME_PAGE_DIRECTORY_START;
  uint8 *pde_start_bytes = (uint8 *)BOOT_TIME_PAGE_DIRECTORY_START;
  
  for (i=0;i<PAGE_SIZE;++i)
    pde_start_bytes[i] = 0;
  
  for (i=0;i<5;++i)
  {
    pde_start[i].pde4m.present = 1;
    pde_start[i].pde4m.writeable = 1;
    pde_start[i].pde4m.user_access = 1;
    pde_start[i].pde4m.use_4_m_pages = 1;
    pde_start[i].pde4m.page_base_address = i;
  }

  for (i=0;i<5;++i)
  {
    pde_start[i+512].pde4m.present = 1;
    pde_start[i+512].pde4m.writeable = 0;
    pde_start[i+512].pde4m.user_access = 1;
    pde_start[i+512].pde4m.use_4_m_pages = 1;
    pde_start[i+512].pde4m.page_base_address = i;
  }
 
  for (i=0;i<256;++i)
  {
    pde_start[i+768].pde4m.present = 1;
    pde_start[i+768].pde4m.writeable = 1;
    pde_start[i+768].pde4m.user_access = 0;
    pde_start[i+768].pde4m.use_4_m_pages = 1;
    pde_start[i+768].pde4m.page_base_address = i;
  }
 
}

void removeBootTimeIdentMapping()
{
  uint32 i;
 
  page_directory_entry *pde_start = (page_directory_entry*)BOOT_TIME_PAGE_DIRECTORY_START;

  for (i=0;i<5;++i)
  {
    pde_start[i].pde4m.present=0;
  }
}

void handleInterrupt(void *regs)
{
}
