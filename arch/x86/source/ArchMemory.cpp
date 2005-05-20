//----------------------------------------------------------------------
//  $Id: ArchMemory.cpp,v 1.4 2005/05/20 11:58:10 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: ArchMemory.cpp,v $
//  Revision 1.3  2005/05/19 20:04:16  btittelbach
//  Much of this still needs to be moved to arch
//
//  Revision 1.2  2005/05/19 19:35:30  btittelbach
//  ein bisschen Arch Memory
//
//  Revision 1.1  2005/05/16 20:37:51  nomenquis
//  added ArchMemory for page table manip
//
//----------------------------------------------------------------------

#include "ArchMemory.h"

extern "C" uint32 kernel_page_directory_start;

void ArchMemory::initNewPageDirectory(uint32 physical_page_to_use)
{
  page_directory_entry *new_page_directory = (page_directory_entry*) get3GBAdressOfPPN(physical_page_to_use);
  
  ArchCommon::memcpy((pointer) new_page_directory,(pointer) &kernel_page_directory_start, PAGE_SIZE);
  for (uint32 p = 0; p< 512; ++p) //we're concerned with first two gig, rest stays as is
  {
    new_page_directory[p].pde4k.present=0;
    new_page_directory[p].pde4m.present=0;
  }
}

void ArchMemory::initNewPageTable(uint32 physical_page_to_use)
{
  pointer addr = get3GBAdressOfPPN(physical_page_to_use);
  ArchCommon::bzero(addr,PAGE_SIZE);
}

void ArchMemory::insertPTE(uint32 physical_page_directory_page, uint32 pde_vpn, uint32 physical_page_table_page)
{
  page_directory_entry *which_page_directory = (page_directory_entry *) get3GBAdressOfPPN(physical_page_directory_page);
  which_page_directory[pde_vpn].pde4k.page_table_base_address = physical_page_table_page; 
  which_page_directory[pde_vpn].pde4k.present = 1;
	which_page_directory[pde_vpn].pde4k.writeable = 1;
	which_page_directory[pde_vpn].pde4k.user_access = 1;
}

void ArchMemory::mapPage(uint32 physical_page_directory_page, uint32 virtual_page, uint32 physical_page, uint32 user_access)
{
  page_directory_entry *which_page_directory = (page_directory_entry *) get3GBAdressOfPPN(physical_page_directory_page);
  page_table_entry *pte_base = (page_table_entry *) get3GBAdressOfPPN(which_page_directory[virtual_page/PAGE_TABLE_ENTRIES].pde4k.page_table_base_address);
  uint32 pte_vpn=virtual_page % PAGE_TABLE_ENTRIES;
  pte_base[pte_vpn].present = 1;
  pte_base[pte_vpn].writeable = 1;
  pte_base[pte_vpn].user_access = user_access;
  pte_base[pte_vpn].accessed = 0;
  pte_base[pte_vpn].dirty = 0;
  pte_base[pte_vpn].page_base_address = physical_page;
}

uint32 ArchMemory::removePTE(uint32 physical_page_directory_page, uint32 pde_vpn)
{
  page_directory_entry *which_page_directory = (page_directory_entry *) get3GBAdressOfPPN(physical_page_directory_page);
  which_page_directory[pde_vpn].pde4k.present = 0;
  return which_page_directory[pde_vpn].pde4k.page_table_base_address;
}

uint32 ArchMemory::unmapPage(uint32 physical_page_directory_page, uint32 virtual_page)
{
  page_directory_entry *which_page_directory = (page_directory_entry *) get3GBAdressOfPPN(physical_page_directory_page);
  page_table_entry *pte_base = (page_table_entry *) get3GBAdressOfPPN(which_page_directory[virtual_page/PAGE_TABLE_ENTRIES].pde4k.page_table_base_address);
  uint32 pte_vpn=virtual_page % PAGE_TABLE_ENTRIES;
  pte_base[pte_vpn].present = 0;
  return pte_base[pte_vpn].page_base_address;
}
