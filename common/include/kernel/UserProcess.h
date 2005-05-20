//----------------------------------------------------------------------
//  $Id: UserProcess.h,v 1.5 2005/05/20 14:07:20 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: UserProcess.h,v $
//  Revision 1.4  2005/05/20 11:58:10  btittelbach
//  much much nicer UserProcess Page Management, but still things to do
//
//  Revision 1.3  2005/05/19 20:04:16  btittelbach
//  Much of this still needs to be moved to arch
//
//  Revision 1.2  2005/05/19 19:35:30  btittelbach
//  ein bisschen Arch Memory
//
//  Revision 1.1  2005/05/19 15:45:38  btittelbach
//  Struktur zum Verwalten von UserProcessen, noch unfertig
//
//
//----------------------------------------------------------------------


#ifndef _USERPROCESS_H
#define _USERPROCESS_H

#include "paging-definitions.h"
#include "mm/PageManager.h"
#include "mm/page-table-manip.h"
#include "../../arch/common/include/ArchMemory.h"
//#include "../../arch/arch/include/panic.h"

#ifdef __cplusplus
extern "C"
{
#endif



#ifdef __cplusplus
}
#endif

uint32 const UserStackSize_ = 4U*1024U; //4k

class UserProcess
{
public:
  //~ UserProcess(); //will need some way to get code from somewhere (uint32 inode ??)
  //~ ~UserProcess();
  
  void installUserSpaceTable();
  
private: 
  uint32 calculateSizeNeeded(); //will need some arguments
  void loadELF(); //will need some arguments
  pointer allignAdress(pointer addr);
  
  //member variables:
  uint32 page_directory_ppn_;
  uint32 ppn_of_pagetable_[3]; //should propably be a list instead, WHERE IST THE PROMISED LIST ?
  uint32 number_of_code_pages_;
  uint32 number_of_heap_pages_;
  uint32 number_of_stack_pages_;
  pointer va_code_start_;
  pointer va_heap_start_;
  pointer va_stack_start_;
};

#endif /* _USERPROCESS_H */
