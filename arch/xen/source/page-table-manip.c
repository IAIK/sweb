/**
 * $Id: page-table-manip.c,v 1.2 2005/08/11 16:55:47 nightcreature Exp $
 *
 * $Log: page-table-manip.c,v $
 * Revision 1.1  2005/08/01 08:27:59  nightcreature
 * to satisfy linker
 *
 *
 */

#include "types.h"
#include "page-table-manip.h"
#include "hypervisor.h"


void switchToPageDirectory(uint32 physical_page_directory_page)
{
//  switchToPageDirectoryPointer((page_directory_entry*) (physical_page_directory_page << PAGE_INDEX_OFFSET_BITS));
  //xen: do it with HYPERVISOR_mmu_update!
  //HYPERVISOR_mmu_update(mmu_update_t *req, int count, int *success_count);
  int32 success_count;
  mmu_update_t update;
  update.val = MMUEXT_NEW_BASEPTR;

  //as we have pseudo pages we propbably should translate it to real machine memory ;-)

  
  
  //CHECK: not sure about the following: maybe should shift page 2 bits before?
  update.ptr = MMU_EXTENDED_COMMAND | physical_page_directory_page;
    
  //HYPERVISOR_mmu_update(&update, 1, &success_count);
}

// void switchToPageDirectoryPointer(page_directory_entry *pde)
// {
// }
