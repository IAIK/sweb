#ifndef _BD_DEVICE_DRIVER_
#define _BD_DEVICE_DRIVER_

#include "arch_bd_request.h"
#include "FiFo.h"

class BDDriver
{
  public:
    uint32 addRequest( BDRequest * ) 
    {
      return 0xFFFFFFFF;
    };
    
    int32 readSector ( uint32, uint32 )
    {
      return -1;
    };
    
    int32 writeSector ( uint32, uint32, void *  )
    {
      return -1;
    };

    uint32 getNumSectors( )
    {
      return 0;
    };

    uint32 getSectorSize( )
    {
      return 0;
    };
    
    void serviceIRQ( void )
    {
      return;
    };

    uint16 irq;
};

#endif

