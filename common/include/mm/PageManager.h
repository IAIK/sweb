//----------------------------------------------------------------------
//   $Id: PageManager.h,v 1.6 2005/05/19 20:04:17 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: PageManager.h,v $
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

#define PAGE_RESERVED static_cast<uint32>(1<<31)
#define PAGE_KERNEL static_cast<uint32>(1<<30)
#define PAGE_USERSPACE static_cast<uint32>(1<<30)


#define PAGE_FREE static_cast<uint32>(0)

class PageManager
{
public:
  
  /**
   * Creates a new instance (and THE ONLY ONE) of our page manager
   * This one will also automagically initialise itself accordingly
   * i.e. mark pages used by the kernel already as used
   * @param next_usable_address Pointer to memory the page manager can use
   * @return 0 on failure, otherwise a pointer to the next free memory location
   */
  static pointer createPageManager(pointer next_usable_address);
  static PageManager *instance() {return instance_;}

  uint32 getTotalNumPages() const;

  uint32 getFreePhysicalPage(uint32 type = PAGE_USERSPACE); //also marks page as used

  void freePage(uint32 page_number);


private:
  
  uint32 getSizeOfMemoryUsed() const;

  PageManager(pointer start_of_structure);
  PageManager(PageManager const&){};
  //PageManager &operator=(PageManager const&){};
  static PageManager* instance_;
  
  uint32 *page_usage_table_;
  uint32 number_of_pages_;
    
};







#endif
