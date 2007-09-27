/**
 * @file page-table-manip.c
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
  // asm volatile("movl %0, %%cr3\n"
              // :
              // : "m"(pde));
  asm volatile("movl %0, %%eax\n"
               "movl %%eax, %%cr3\n"
              :
              : "m"(pde)
              : "eax");
}
