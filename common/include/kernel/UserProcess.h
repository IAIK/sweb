//----------------------------------------------------------------------
//  $Id: UserProcess.h,v 1.2 2005/05/19 19:35:30 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: UserProcess.h,v $
//  Revision 1.1  2005/05/19 15:45:38  btittelbach
//  Struktur zum Verwalten von UserProcessen, noch unfertig
//
//
//----------------------------------------------------------------------


#ifndef _USERPROCESS_H
#define _USERPROCESS_H

#include "paging-definitions.h"
#include "mm/PageManager.h"
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
  UserProcess(); //will need some way to get code from somewhere (uint32 inode ??)
  ~UserProcess();
  
  void installUserSpaceTable();
  
  private: 
  uint32 calculateSizeNeeded(); //will need some arguments
  void loadELF(); //will need some arguments
  pointer allignAdress(pointer addr);
  
  uint32 ppn_of_pagetable_[2]; //should propably be a list instead
  page_table_entry *page_table_;    //code and heap, 4mb limit for now...
  page_table_entry *stack_page_table_; //stack
  uint32 number_of_code_pages_;
  uint32 number_of_heap_pages_;
  uint32 number_of_stack_pages_;
  pointer va_code_start_;  //make it simple and the code readable for now
  pointer va_heap_start_;
  pointer va_stack_start_;
  //other stuff
};

#endif /* _USERPROCESS_H */
