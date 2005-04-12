/**
 * $Id: init_boottime_pagetables.c,v 1.1 2005/04/12 17:46:44 nomenquis Exp $
 *
 * $Log:  $
 *
 */

#include "types.h"
#include "boot-time.h"
#include "paging-definitions.h"


#define BOOT_TIME_PAGE_DIRECTORY_START (1024*1024*4)


void initialiseBootTimePaging()
{
  uint32 i,k;
  
  uint32 *pde_start = (uint32*)BOOT_TIME_PAGE_DIRECTORY_START;
  
  // zero out the page dir, this will mark all entries as invalid
  for (i=0;i<PAGE_SIZE/sizeof(uint32);++i)
  {
    pde_start[i] = 0;
  }
  
  // identity map 1-32mb
  for (i = 0 ; i < 8; ++i)
  {
    unsigned pde_entry = 0;

    unsigned *pte_entry = (unsigned *)(BOOT_TIME_PAGE_DIRECTORY_START + (PAGE_SIZE * (i + 1)));

    pde_entry = ((unsigned)pte_entry);
    pde_entry |= PAGE_PRESENT | PAGE_WRITEABLE |  PAGE_USER_ACCESS;

    pde_start[i] = pde_entry;

    for (k = 0; k < PAGE_SIZE / sizeof(unsigned); ++k)
    {
      unsigned page_num = i * (PAGE_SIZE / sizeof(unsigned)) + k;
      unsigned pte = 0;
      pte = page_num << PAGE_INDEX_OFFSET_BITS;
      pte |= PAGE_PRESENT | PAGE_WRITEABLE |  PAGE_USER_ACCESS;
      pte_entry[k] = pte;
    }
  }
  
  // map 0-16 meg to 2 gig
  for (i = 0; i < 4; ++i)
  {
    unsigned pde_entry = 0;

    unsigned *pte_entry = (unsigned *)(BOOT_TIME_PAGE_DIRECTORY_START + (PAGE_SIZE * (i+9)));

    pde_entry = ((unsigned)pte_entry);
    pde_entry |= PAGE_PRESENT | PAGE_WRITEABLE |  PAGE_USER_ACCESS;

    pde_start[i+512] = pde_entry;

    for (k=0;k<PAGE_SIZE/sizeof(unsigned);++k)
    {
      unsigned page_num = 0x100 + i*(PAGE_SIZE/sizeof(unsigned)) + k;
      unsigned pte = 0;
      pte = page_num << PAGE_INDEX_OFFSET_BITS;
      pte |= PAGE_PRESENT | PAGE_WRITEABLE |  PAGE_USER_ACCESS | PAGE_PINNED;
      pte_entry[k] = pte;
    }
  }

  // map first gig to 3 gig
  for (i = 0; i < 256; ++i)
  {
    unsigned pde_entry = 0;

    unsigned *pte_entry = (unsigned *)(BOOT_TIME_PAGE_DIRECTORY_START + (PAGE_SIZE * (i+17+248)));

    pde_entry = ((unsigned)pte_entry);
    pde_entry |= PAGE_PRESENT | PAGE_WRITEABLE |  PAGE_USER_ACCESS;

    pde_start[i+768] = pde_entry;

    for (k=0 ; k < PAGE_SIZE / sizeof(unsigned); ++k)
    {
      unsigned page_num = i*(PAGE_SIZE/sizeof(unsigned)) + k;
      unsigned pte = 0;
      pte = page_num << PAGE_INDEX_OFFSET_BITS;
      pte |= PAGE_PRESENT | PAGE_WRITEABLE |  PAGE_USER_ACCESS | PAGE_PINNED;
      pte_entry[k] = pte;
    }
  }

}

void freeBootTimePaging()
{
  // here we should tell our page manager that all of our boottime pages are free now
}
