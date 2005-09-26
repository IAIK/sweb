//----------------------------------------------------------------------
//   $Id: Loader.h,v 1.6 2005/09/26 15:29:05 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: Loader.h,v $
//  Revision 1.5  2005/08/26 12:01:25  nomenquis
//  pagefaults in userspace now should really really really work
//
//  Revision 1.4  2005/07/21 19:08:40  btittelbach
//  Jö schön, Threads u. Userprozesse werden ordnungsgemäß beendet
//  Threads können schlafen, Mutex benutzt das jetzt auch
//  Jetzt muß nur der Mutex auch überall verwendet werden
//
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


  /** Ok, this one is buggy, it assumes that each load segment is page aligned
    * which definitely is not the case with elf
    */
  //void loadOnePage(uint32 virtual_address);


  void loadOnePageSafeButSlow(uint32 virtual_address);

private:

  uint8 *file_image_;
  Thread *thread_;
  uint32 page_dir_page_;
};



#endif
