/**
 * $Id: page-table-manip.c,v 1.2 2005/05/20 12:42:55 btittelbach Exp $
 *
 * $Log: page-table-manip.c,v $
 * Revision 1.1  2005/04/12 17:46:44  nomenquis
 * added lots of files
 *
 *
 */

#include "types.h"
#include "page-table-manip.h"


void switchToPageTable(page_directory_entry *pde)
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
