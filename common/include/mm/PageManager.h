//----------------------------------------------------------------------
//   $Id: PageManager.h,v 1.9 2005/11/21 13:26:27 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: PageManager.h,v $
//  Revision 1.8  2005/09/27 21:24:43  btittelbach
//  +IF=1 in PageFaultHandler
//  +Lock in PageManager
//  +readline/gets Bugfix
//  +pseudoshell bugfix
//
//  Revision 1.7  2005/09/24 13:30:20  btittelbach
//  4m page support
//
//  Revision 1.6  2005/05/19 20:04:17  btittelbach
//  Much of this still needs to be moved to arch
//
//  Revision 1.5  2005/05/19 15:43:43  btittelbach
//  Ansätze für eine UserSpace Verwaltung
//
//  Revision 1.4  2005/04/25 23:23:49  btittelbach
//  nothing really
//
//  Revision 1.3  2005/04/23 12:52:26  nomenquis
//  fixes
//
//  Revision 1.2  2005/04/23 12:43:09  nomenquis
//  working page manager
//
//  Revision 1.1  2005/04/20 21:35:33  nomenquis
//  started to implement page manager
//
//----------------------------------------------------------------------

#ifndef _PAGEMANAGER_H_
#define _PAGEMANAGER_H_

#include "types.h"
#include "paging-definitions.h"
#include "new.h"
#include "kernel/Mutex.h"

typedef uint8 puttype;
#define PAGE_RESERVED static_cast<puttype>(1<<0)
#define PAGE_KERNEL static_cast<puttype>(1<<1)
#define PAGE_USERSPACE static_cast<puttype>(1<<2)


#define PAGE_FREE static_cast<puttype>(0)

//-----------------------------------------------------------
/// PageManager is in issence a BitMap managing free or used pages of size PAGE_SIZE only
class PageManager
{
public:
  
//-----------------------------------------------------------
/// Creates a new instance (and THE ONLY ONE) of our page manager
/// This one will also automagically initialise itself accordingly
/// i.e. mark pages used by the kernel already as used
/// @param next_usable_address Pointer to memory the page manager can use
/// @return 0 on failure, otherwise a pointer to the next free memory location
  static pointer createPageManager(pointer next_usable_address);
  static PageManager *instance() {return instance_;}

//-----------------------------------------------------------
/// @return number of 4k Pages avaible to sweb. 
/// ((size of the first usable memory region or max 4Gb) / PAGESIZE)
  uint32 getTotalNumPages() const;

//-----------------------------------------------------------
/// returns the number of the next free Physical Page
/// and marks that Page as used. 
/// @param type can be either PAGE_USERSPACE (default) or PAGE_KERNEL
/// (passing PAGE_RESERVED or PAGE_FREE would obviously not make much sense,
///  since the page then either can neve be free'd again or will be given out 
///  again)
  uint32 getFreePhysicalPage(uint32 type = PAGE_USERSPACE); //also marks page as used

//-----------------------------------------------------------
/// marks physical page <page_number> as free, if it was used in
/// user or kernel space.
/// @param page_number Physcial Page to mark as unused
  void freePage(uint32 page_number);

//-----------------------------------------------------------
/// call this after initializing the KernelMemoryManager and before
/// starting Interrupts to ensure Mutual Exclusion
  void startUsingSyncMechanism() {lock_=new Mutex();}

private:
  
  uint32 getSizeOfMemoryUsed() const;

  PageManager(pointer start_of_structure);
  PageManager(PageManager const&){};
  //PageManager &operator=(PageManager const&){};
  static PageManager* instance_;
  
  puttype  *page_usage_table_;
  uint32 number_of_pages_;
    
  Mutex *lock_;
    
};







#endif
