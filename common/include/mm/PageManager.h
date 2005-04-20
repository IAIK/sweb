//----------------------------------------------------------------------
//   $Id: PageManager.h,v 1.1 2005/04/20 21:35:33 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: $
//----------------------------------------------------------------------

#ifndef _PAGEMANAGER_H_
#define _PAGEMANAGER_H_

#include "types.h"


class PageManager
{
public:
  
  /**
   * Creates a new instance (and THE ONLY ONE) of our page manager
   * This one will also automagically initialise itself accordingly
   * i.e. mark pages used by the kernel already as used
   * @param number_of_pages The number of physical Pages we have
   * @return 0 on sucess something else otherwise
   */
  static uint32 createPageManager(uint32 const &number_of_pages);


private:
  
  PageManager(uint32 number_of_pages);
  PageManager(PageManager const&){};
  PageManager &operator=(PageManager const&){};
  static PageManager* instance_;
  
};







#endif
