//----------------------------------------------------------------------
//   $Id: Loader.h,v 1.4 2005/07/21 19:08:40 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: Loader.h,v $
//  Revision 1.3  2005/07/12 21:05:38  btittelbach
//  Lustiges Spielen mit UserProgramm Terminierung
//
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
  void cleanupUserspaceAddressSpace();

  uint32 loadExecutableAndInitProcess();
  void loadOnePage(uint32 virtual_address);
private:

  uint8 *file_image_;
  Thread *thread_;
  uint32 page_dir_page_;
};



#endif
