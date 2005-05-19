//----------------------------------------------------------------------
//  $Id: UserProcess.cpp,v 1.2 2005/05/19 20:04:17 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: UserProcess.cpp,v $
//  Revision 1.1  2005/05/19 15:45:38  btittelbach
//  Struktur zum Verwalten von UserProcessen, noch unfertig
//
//
//----------------------------------------------------------------------

#include "UserProcess.h"

UserProcess::UserProcess()
{
  uint32 code_size = calculateSizeNeeded(); //give arguments from Constructor
  number_of_code_pages_  = code_size / PAGE_SIZE + ((code_size % PAGE_SIZE)?1:0); 
  number_of_heap_pages_  = 1;
  number_of_stack_pages_ = 1;
  va_code_start_  = 0;
  va_heap_start_  = allignAdress(((pointer) va_code_start_ + code_size));
  va_stack_start_ = 2U*1024U*1024U*1024U-1;  //starts at upper end of 2g and grows down
  
  if (number_of_code_pages_ > 1023)
    kpanict((uint8*) "UserProcess: To many pages needed, not implemented yet\n");
  
  ppn_of_pagetable_[0] = PageManager::instance()->getFreePhysicalPage(); //page directory
  ppn_of_pagetable_[1] = PageManager::instance()->getFreePhysicalPage(); //code & heap
  ppn_of_pagetable_[2] = PageManager::instance()->getFreePhysicalPage(); //stack
  
  page_directory_ = ArchMemory::initNewPageDirectory(ppn_of_pagetable_[0]);
  
  //evil, but ok
  page_table_ = ArchMemory::initNewPageTable(ppn_of_pagetable_[1]);
  stack_page_table_ = ArchMemory::initNewPageTable(ppn_of_pagetable_[2]);
   
  uint32 p=0;
  for (p=0; p< number_of_code_pages_+1; ++p) //+1 for some initial heap space
  {
    uint32 ppn = PageManager::instance()->getFreePhysicalPage();  
    //FIXXME: -> arch :
    uint32 paddr = ppn*PAGE_SIZE;
    page_table_[p].present = 1;
    page_table_[p].writeable = 1;
    page_table_[p].user_access = 1;
    page_table_[p].accessed = 0;
    page_table_[p].dirty = 0;
    page_table_[p].page_base_address = paddr >> 12; //only highest 20 bit of paddr matter (FIXXME -> Arch ?)
  }
  
  {
    //initial stack page:
    uint32 ppn = PageManager::instance()->getFreePhysicalPage();
    //FIXXME: -> arch :
    uint32 paddr = ppn*PAGE_SIZE;
    stack_page_table_[1023].present = 1;
    stack_page_table_[1023].writeable = 1;
    stack_page_table_[1023].user_access = 1;
    stack_page_table_[1023].accessed = 0;
    stack_page_table_[1023].dirty = 0;
    stack_page_table_[1023].page_base_address = paddr >> 12; //only highest 20 bit of paddr matter (FIXXME -> Arch ?)    
  }
  
  installUserSpaceTable();
  loadELF();
}

UserProcess::~UserProcess()
{
  //free code pages
  //free pages that were allocated for heap
  uint32 p=0;
  for (p=0; p< number_of_code_pages_; ++p)
    if (page_table_[p].present)
      PageManager::instance()->freePage((page_table_[p].page_base_address << 12) / PAGE_SIZE); //belongs into arch.... FIXXME
  
  //free pages for stack
  for (p=1024; p!= 0; --p) //ok page 0 wont be freed here: FIXXME
    if (stack_page_table_[p].present)    //should be just one
      PageManager::instance()->freePage((stack_page_table_[p].page_base_address << 12) / PAGE_SIZE); //belongs into arch.... FIXXME

  //free page where pageTable is
  PageManager::instance()->freePage(ppn_of_pagetable_[0]);
  PageManager::instance()->freePage(ppn_of_pagetable_[1]);
}
  
void UserProcess::installUserSpaceTable()
{
 // ArchMemory::instance()->mapPage(..,0,...);
}

uint32 UserProcess::calculateSizeNeeded()
{
  //FIXXME
  return 543;
}

void UserProcess::loadELF()
{
  uint8 *hello_world_exe_content= (uint8*) "?syscalls?";
  uint8 *someloop_exe_content= (uint8*) "";
  
  //FIXXME
}


pointer UserProcess::allignAdress(pointer addr)
{
  addr += (4 - (addr % 4)) % 4;
  return addr;
}
