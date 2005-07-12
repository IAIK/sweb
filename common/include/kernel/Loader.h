//----------------------------------------------------------------------
//   $Id: Loader.h,v 1.3 2005/07/12 21:05:38 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: Loader.h,v $
//  Revision 1.2  2005/06/14 18:22:37  btittelbach
//  RaceCondition anfälliges LoadOnDemand implementiert,
//  sollte optimalerweise nicht im InterruptKontext laufen
//
//  Revision 1.1  2005/05/31 18:25:49  nomenquis
//  forgot to add loader
//
//----------------------------------------------------------------------

#ifndef __LOADER_H__
#define __LOADER_H__

#include "types.h"
#include "Thread.h"
#include "Scheduler.h"

class Loader
{
public:
  Loader(uint8* file_image, Thread *thread);

  void initUserspaceAddressSpace();

  uint32 loadExecutableAndInitProcess();
  void loadOnePage(uint32 virtual_address);
private:

  Thread *thread_;
  uint8 *file_image_;
  uint32 page_dir_page_;
};



#endif
