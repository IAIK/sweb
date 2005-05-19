//----------------------------------------------------------------------
//  $Id: ArchMemory.cpp,v 1.2 2005/05/19 19:35:30 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchMemory.cpp,v $
//  Revision 1.1  2005/05/16 20:37:51  nomenquis
//  added ArchMemory for page table manip
//
//----------------------------------------------------------------------

#include "ArchMemory.h"

extern "C" uint32 kernel_page_directory_start;

pointer ArchMemory::initNewPageDirectory(uint32 physicalPageToUse)
{
  page_directory_entry *new_page_directory = (page_directory_entry*) get3GBAdressOfPPN(physicalPageToUse);
  
  ArchCommon::memcpy((pointer) new_page_directory,(pointer) &kernel_page_directory_start, PAGE_SIZE);
  for (uint32 p = 0; p< 512; ++p) //we're concerned with first two gig, rest stays as is
  {
    new_page_directory[p].pde4k.present=0;
    new_page_directory[p].pde4m.present=0;
  }
  return (pointer) new_page_directory;
}


void ArchMemory::insertPTE(page_directory_entry *whichPageDirectory, uint32 pageDirectoryPage, pointer pageTable)
{
  if (pageTable && 0x00000fff)
    kpanict((uint8*) "insertPTE: We got a problem here....PageTable Address not aligned");
  whichPageDirectory[pageDirectoryPage].pde4k.page_table_base_address = (uint32) (pageTable >> 12);
  whichPageDirectory[pageDirectoryPage].pde4k.present = 1;
	whichPageDirectory[pageDirectoryPage].pde4k.writeable = 1;
	whichPageDirectory[pageDirectoryPage].pde4k.user_access = 1;
}
