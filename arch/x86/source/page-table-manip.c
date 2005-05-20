/**
 * $Id: page-table-manip.c,v 1.3 2005/05/20 14:18:29 btittelbach Exp $
 *
 * $Log: page-table-manip.c,v $
 * Revision 1.2  2005/05/20 12:42:55  btittelbach
 * Switching PDE's
 *
 * Revision 1.1  2005/04/12 17:46:44  nomenquis
 * added lots of files
 *
 *
 */

#include "types.h"
#include "page-table-manip.h"


void switchToPageDirectory(uint32 physical_page_directory_page)
{
  switchToPageDirectoryPointer((page_directory_entry*) (physical_page_directory_page << PAGE_INDEX_OFFSET_BITS));
}

void switchToPageDirectoryPointer(page_directory_entry *pde)
{
  //~ asm volatile("movl %0, %%cr3\n"
              //~ : 
              //~ : "m"(pde));
  asm volatile("movl %0, %%eax\n"
               "movl %%eax, %%cr3\n"
              : 
              : "m"(pde) 
              : "eax");  
}
