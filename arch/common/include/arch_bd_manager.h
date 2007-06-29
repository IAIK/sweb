/********************************************************************
*
*    $Id: arch_bd_manager.h,v 1.2 2006/09/19 20:40:23 aniederl Exp $
*    $Log: arch_bd_manager.h,v $
*    Revision 1.1  2005/09/20 16:24:38  rotho
*    moved arch_bd_manager.h to arch/common/include/
*
*    Revision 1.2  2005/09/18 20:46:52  nelles
*
*     Committing in .
*
*     Modified Files:
*     	arch/x86/include/arch_bd_ata_driver.h
*     	arch/x86/include/arch_bd_ide_driver.h
*     	arch/x86/include/arch_bd_manager.h
*     	arch/x86/include/arch_bd_request.h
*     	arch/x86/include/arch_bd_virtual_device.h
*     	arch/x86/source/arch_bd_ata_driver.cpp
*     	arch/x86/source/arch_bd_ide_driver.cpp
*     	arch/x86/source/arch_bd_manager.cpp
*     	arch/x86/source/arch_bd_virtual_device.cpp
*     ----------------------------------------------------------------------
*
********************************************************************/

#ifndef _BD_DEVICE_MANAGER_
#define _BD_DEVICE_MANAGER_

#include "arch_bd_request.h"
#include "arch_bd_virtual_device.h"
#include "../../common/include/util/List.h"

//extern template class List;

class BDManager
{
public:
  BDManager();
  ~BDManager();
  
  static BDManager* getInstance        ( void );
  
  void              doDeviceDetection  ( void );
  
  void              addVirtualDevice   ( BDVirtualDevice* dev );
  
  BDVirtualDevice*  getDeviceByNumber  ( uint32 dev_num  );

  BDVirtualDevice*  getDeviceByName    ( const char * dev_name );

  uint32            getNumberOfDevices ( void );
  
  void              addRequest         ( BDRequest * );
  
  void              serviceIRQ         ( uint32  );
  
  bool              probeIRQ;
  
private:
  List< BDVirtualDevice *> device_list_;
    
protected:
  static BDManager * instance_;  
};

#endif

