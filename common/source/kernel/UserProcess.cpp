//----------------------------------------------------------------------
//  $Id: UserProcess.cpp,v 1.7 2005/05/20 14:07:21 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: UserProcess.cpp,v $
//  Revision 1.6  2005/05/20 12:42:56  btittelbach
//  Switching PDE's
//
//  Revision 1.5  2005/05/20 11:58:10  btittelbach
//  much much nicer UserProcess Page Management, but still things to do
//
//  Revision 1.3  2005/05/19 21:22:10  btittelbach
//  Executables as CString for initial Testing without FileSystem
//
//  Revision 1.2  2005/05/19 20:04:17  btittelbach
//  Much of this still needs to be moved to arch
//
//  Revision 1.1  2005/05/19 15:45:38  btittelbach
//  Struktur zum Verwalten von UserProcessen, noch unfertig
//
//
//----------------------------------------------------------------------

#include "UserProcess.h"

//~ UserProcess::UserProcess()
//~ {
  //~ uint32 code_size = calculateSizeNeeded(); //give arguments from Constructor
  //~ number_of_code_pages_  = code_size / PAGE_SIZE + ((code_size % PAGE_SIZE)?1:0); 
  //~ number_of_heap_pages_  = 1;
  //~ number_of_stack_pages_ = UserStackSize_ / PAGE_SIZE;
  //~ va_code_start_  = 0;
  //~ va_heap_start_  = allignAdress(((pointer) va_code_start_ + code_size));
  //~ va_stack_start_ = 2U*1024U*1024U*1024U-4;  //starts at upper end of 2g and grows down
  
  //~ if (number_of_code_pages_ + number_of_heap_pages_> 1024)
    //~ kpanict((uint8*) "UserProcess: To many pages needed, not implemented yet, need dynamic List\n");
  
  //~ page_directory_ppn_ = PageManager::instance()->getFreePhysicalPage(); //page directory
  //~ ppn_of_pagetable_[0] = PageManager::instance()->getFreePhysicalPage(); //code & heap, only 1 for now
  //~ ppn_of_pagetable_[1] = PageManager::instance()->getFreePhysicalPage(); //stack
  
  //~ ArchMemory::initNewPageDirectory(page_directory_ppn_);
  //~ ArchMemory::initNewPageTable(ppn_of_pagetable_[0]);
  //~ ArchMemory::insertPTE(page_directory_ppn_, (va_code_start_/PAGE_SIZE/PAGE_TABLE_ENTRIES) ,ppn_of_pagetable_[0]);
  //~ ArchMemory::initNewPageTable(ppn_of_pagetable_[1]);
  //~ ArchMemory::insertPTE(page_directory_ppn_, (va_stack_start_/PAGE_SIZE/PAGE_TABLE_ENTRIES) ,ppn_of_pagetable_[1]);
   
  //~ uint32 vpn=0;
  //~ for (vpn=0; vpn< number_of_code_pages_+number_of_heap_pages_; ++vpn)
  //~ {
    //~ uint32 ppn = PageManager::instance()->getFreePhysicalPage();  
    //~ ArchMemory::mapPage(page_directory_ppn_,vpn,ppn,1);
  //~ }
  
  //~ uint32 last_vpn=(va_stack_start_/PAGE_SIZE);
  //~ for (vpn=last_vpn+1-number_of_stack_pages_; vpn< last_vpn+1; ++vpn)
  //~ {
    //~ uint32 ppn = PageManager::instance()->getFreePhysicalPage();
    //~ ArchMemory::mapPage(page_directory_ppn_,vpn,ppn,1);
  //~ }
  
  //~ installUserSpaceTable();
  //~ loadELF();
//~ }

//~ UserProcess::~UserProcess()
//~ {
  //~ //free code pages
  //~ //free pages that were allocated for heap
  
  //~ uint32 vpn=0;
  //~ for (vpn=0; vpn< number_of_code_pages_+number_of_heap_pages_; ++vpn)
  //~ {
    //~ uint32 ppn = ArchMemory::unmapPage(page_directory_ppn_,vpn);
    //~ PageManager::instance()->freePage(ppn);
  //~ }
  
  //~ uint32 last_vpn=(va_stack_start_/PAGE_SIZE);
  //~ for (vpn=last_vpn-number_of_stack_pages_+1; vpn < last_vpn+1; ++vpn)
  //~ {
    //~ uint32 ppn = ArchMemory::unmapPage(page_directory_ppn_,vpn);
    //~ PageManager::instance()->freePage(ppn);
  //~ }
  
  //~ //free page where pageTables are
  //~ for (uint32 p=0; p<2; ++p)
    //~ PageManager::instance()->freePage(ppn_of_pagetable_[p]);

  //~ PageManager::instance()->freePage(page_directory_ppn_);
//~ }
  
void UserProcess::installUserSpaceTable()
{
  switchToPageTable((page_directory_entry*) (page_directory_ppn_ << PAGE_INDEX_OFFSET_BITS));
}

uint32 UserProcess::calculateSizeNeeded()
{
  //FIXXME
  return 543;
}

void UserProcess::loadELF()
{
  uint8 *hello_world_exe_content= (uint8*) "?syscalls?";
  uint8 *someloop_exe_content= (uint8*) "\177\105\114\106\1\1\1\0\0\0\0\0\0\0\0\0\1\0\3\0\1\0\0\0\0\0\0\0\0\0\0\0\20\1\0\0\0\0\0\0\64\0\0\0\0\0\50\0\11\0\6\0\125\211\345\203\354\10\203\344\360\270\0\0\0\0\51\304\307\105\374\7\0\0\0\307\105\370\0\0\0\0\203\175\370\17\176\2\353\14\215\105\370\321\40\215\105\370\377\0\353\354\213\105\374\311\303\0\0\107\103\103\72\40\50\107\116\125\51\40\63\56\63\56\65\55\62\60\60\65\60\61\63\60\40\50\107\145\156\164\157\157\40\114\151\156\165\170\40\63\56\63\56\65\56\62\60\60\65\60\61\63\60\55\162\61\54\40\163\163\160\55\63\56\63\56\65\56\62\60\60\65\60\61\63\60\55\61\54\40\160\151\145\55\70\56\67\56\67\56\61\51\0\0\56\163\171\155\164\141\142\0\56\163\164\162\164\141\142\0\56\163\150\163\164\162\164\141\142\0\56\164\145\170\164\0\56\144\141\164\141\0\56\142\163\163\0\56\156\157\164\145\56\107\116\125\55\163\164\141\143\153\0\56\143\157\155\155\145\156\164\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\33\0\0\0\1\0\0\0\6\0\0\0\0\0\0\0\64\0\0\0\67\0\0\0\0\0\0\0\0\0\0\0\4\0\0\0\0\0\0\0\41\0\0\0\1\0\0\0\3\0\0\0\0\0\0\0\154\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\4\0\0\0\0\0\0\0\47\0\0\0\10\0\0\0\3\0\0\0\0\0\0\0\154\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\4\0\0\0\0\0\0\0\54\0\0\0\1\0\0\0\0\0\0\0\0\0\0\0\154\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\0\0\0\0\74\0\0\0\1\0\0\0\0\0\0\0\0\0\0\0\154\0\0\0\137\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\0\0\0\0\21\0\0\0\3\0\0\0\0\0\0\0\0\0\0\0\313\0\0\0\105\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\0\0\0\0\1\0\0\0\2\0\0\0\0\0\0\0\0\0\0\0\170\2\0\0\200\0\0\0\10\0\0\0\7\0\0\0\4\0\0\0\20\0\0\0\11\0\0\0\3\0\0\0\0\0\0\0\0\0\0\0\370\2\0\0\23\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\0\0\0\0\0\0\0\0\4\0\361\377\0\0\0\0\0\0\0\0\0\0\0\0\3\0\1\0\0\0\0\0\0\0\0\0\0\0\0\0\3\0\2\0\0\0\0\0\0\0\0\0\0\0\0\0\3\0\3\0\0\0\0\0\0\0\0\0\0\0\0\0\3\0\4\0\0\0\0\0\0\0\0\0\0\0\0\0\3\0\5\0\16\0\0\0\0\0\0\0\67\0\0\0\22\0\1\0\0\163\157\155\145\154\157\157\160\56\143\160\160\0\155\141\151\156\0";
  //FIXXME
}


pointer UserProcess::allignAdress(pointer addr)
{
  addr += (4 - (addr % 4)) % 4;
  return addr;
}
