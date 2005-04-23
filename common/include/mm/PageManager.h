//----------------------------------------------------------------------
//   $Id: PageManager.h,v 1.3 2005/04/23 12:52:26 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: PageManager.h,v $
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

#define PAGE_RESERVED (1<<31)
#define PAGE_KERNEL (1<<30)



#define PAGE_FREE (0)

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

  uint32 getTotalNumPages() const;


private:
  
  uint32 getSizeOfMemoryUsed() const;

  PageManager(pointer start_of_structure);
  PageManager(PageManager const&){};
  PageManager &operator=(PageManager const&){};
  static PageManager* instance_;
  
  uint32 *page_usage_table_;
  uint32 number_of_pages_;
    
};







#endif
