/********************************************************************
*
*    $Id: arch_bd_ata_driver.h,v 1.2 2005/09/18 20:46:52 nelles Exp $
*    $Log: arch_bd_ata_driver.h,v $
********************************************************************/

#ifndef _ATA_DEVICE_DRIVER_
#define _ATA_DEVICE_DRIVER_

#include "arch_bd_request.h"
#include "arch_bd_driver.h"
#include "arch_bd_io.h"
#include "FiFo.h"

class ATADriver : public BDDriver, bdio
{
  public:
  
    typedef enum BD_ATA_MODE_ {
      BD_PIO_NO_IRQ,
      BD_PIO,
      BD_DMA,
      BD_UDMA
    } BD_ATA_MODES;
    
  
    uint32 addRequest( BDRequest * );
    
    ATADriver( uint16 baseport, uint16 getdrive, uint16 irqnum );
    ~ATADriver();
    
    int32 readSector     ( uint32, uint32, void * );
    int32 writeSector    ( uint32, uint32, void * );
        
    uint32 getNumSectors ( void )     { return numsec; };
    uint32 getSectorSize ( void )     { return 512; };
    
    void serviceIRQ      ( void );
    void testIRQ         ( );

    uint32 HPC, SPT;        // HEADS PER CYLINDER and SECTORS PER TRACK
    
  private:  
    
    uint16           dd [256];    // read buffer if we need one
    uint32           dd_off;      // read buffer counter

    uint32           numsec;

    uint16           port;
    uint16           drive;
    
    uint32           jiffies;
    
    BD_ATA_MODES     mode; // mode see enum BD_ATA_MODES
    
    FiFo< BDRequest * > * request_queue_;
};

#endif

