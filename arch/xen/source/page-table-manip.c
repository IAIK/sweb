/**
 * $Id: page-table-manip.c,v 1.1 2005/08/01 08:27:59 nightcreature Exp $
 *
 * $Log: page-table-manip.c,v $
 *
 */

#include "types.h"
#include "page-table-manip.h"


void switchToPageDirectory(uint32 physical_page_directory_page)
{
  //switchToPageDirectoryPointer((page_directory_entry*) (physical_page_directory_page << PAGE_INDEX_OFFSET_BITS));
}

void switchToPageDirectoryPointer(page_directory_entry *pde)
{
  //~ asm volatile("movl %0, %%cr3\n"
              //~ : 
              //~ : "m"(pde));

//that is forbidden now...just to calm the linker we leave it in now
  
//   asm volatile("movl %0, %%eax\n"
//                "movl %%eax, %%cr3\n"
//               : 
//               : "m"(pde) 
//               : "eax");  
}
