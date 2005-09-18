#ifndef _BD_DEVICE_MANAGER_
#define _BD_DEVICE_MANAGER_

#include "arch_bd_request.h"
#include "arch_bd_virtual_device.h"
#include "List.h"

class BDManager
{
public:
  BDManager();
  ~BDManager();
  
  static BDManager* getInstance        ( void );
  
  void              doDeviceDetection  ( void );
  
  void              addVirtualDevice   ( BDVirtualDevice* dev );
  
  BDVirtualDevice*  getDeviceByNumber  ( uint32 dev_num  );
  BDVirtualDevice*  getDeviceByName    ( char * dev_name );
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

