//----------------------------------------------------------------------
//   $Id: Loader.h,v 1.1 2005/05/31 18:25:49 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: $
//----------------------------------------------------------------------

#ifndef __LOADER_H__
#define __LOADER_H__

#include "types.h"
#include "Thread.h"

class Loader
{
public:
  Loader(uint8* file_image, Thread *thread);

  void initUserspaceAddressSpace();

  uint32 loadExecutableAndInitProcess();
private:

  Thread *thread_;
  uint8 *file_image_;
  uint32 page_dir_page_;
};



#endif
