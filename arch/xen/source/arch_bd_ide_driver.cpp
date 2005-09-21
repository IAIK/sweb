/********************************************************************
*
*    $Id: arch_bd_ide_driver.cpp,v 1.1 2005/09/21 03:33:52 rotho Exp $
*    $Log: arch_bd_ide_driver.cpp,v $
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

#include "arch_bd_ide_driver.h"
#include "arch_bd_ata_driver.h"
#include "arch_bd_virtual_device.h"
#include "arch_bd_manager.h"

uint32 IDEDriver::doDeviceDetection()
{
   uint32 jiffies = 0;
   uint16 base_port = 0x1F0;
   uint16 base_regport = 0x3F6; 
   uint8 cs = 0;
   uint8 sc;
   uint8 sn;
   uint8 devCtrl;

   // setup register values

   devCtrl = 0x00; // please use interrupts

   // assume there are no devices

   kprintfd("IDEDriver::doDetection:\n", cs);   
    
   for( cs = 0; cs < 4; cs ++)
   {
      kprintfd("IDEDriver::doDetection:Detecting IDE DEV%d:\n", cs);   

      if( cs > 1 )
      {
        base_port = 0x170;
        base_regport = 0x376;
      }

      outbp( base_regport, devCtrl ); // init the device without interupts

      outbp( base_port + 6, (cs % 2 == 0 ? 0xA0 : 0xB0 ) );  

      outbp( base_port + 2, 0x55 );
      outbp( base_port + 3, 0xAA );
      outbp( base_port + 2, 0xAA );
      outbp( base_port + 3, 0x55 );
      outbp( base_port + 2, 0x55 );
      outbp( base_port + 3, 0xAA );

      sc = inbp( base_port + 2 );
      sn = inbp( base_port + 3 );
      if ( ( sc == 0x55 ) && ( sn == 0xAA ) )
      {
        outbp( base_regport , devCtrl | 0x04 ); // RESET
        outbp( base_regport , devCtrl );
        
        while( inbp( base_port + 7 ) != 58 && jiffies ++ < 50000 )
            ;
        
        if( jiffies == 50000 )
          kprintfd("IDEDriver::doDetection: Still busy after reset!\n ");
        else
        {
          outbp( base_port + 6, (cs % 2 == 0 ? 0xA0 : 0xB0 ) );  
   
          uint8 c1 = inbp( base_port + 2 ); 
          uint8 c2 = inbp( base_port + 3 );
   
          if( c1 != 0x01 && c2 != 0x01 )
            kprintfd("IDEDriver::doDetection: Not found after reset ! \n");
          else
          {
            uint8 c3 = inbp( base_port + 7 );
            uint8 c4 = inbp( base_port + 4 );
            uint8 c5 = inbp( base_port + 5 );
  
            if( ( (c4 == 0x14) && (c5 == 0xEB) ) || ( (c4 == 0x69) && (c5 == 0x96) ) )
            {
                kprintfd("IDEDriver::doDetection: Found ATAPI ! \n");
                kprintfd("IDEDriver::doDetection: port: %4X, drive: %d \n", base_port, cs%2);
            
                kprintfd("IDEDriver::doDetection: CDROM not supported \n");
                
                /////////////////////////////
                // CDROM hook goes here
                ///////////////////////////// 
                // 
                // char *name = "ATAX0";
                // name[3] = cs + '0';
                // drv = new CROMDriver ( base_port, cs % 2 );
                // BDVirtualDevice *bdv = new 
                // BDVirtualDevice( drv, 0, drv->getNumSectors(),
                // drv->getSectorSize(), name, true);
                // BDManager::getInstance()->addDevice( bdv );
                //
                /////////////////////////////
            }
            else
            {
              if( c3 != 0 )
              {
                if( ( c4 == 0x00 ) && ( c5 == 0x00 ) )
                {
                    kprintfd("IDEDriver::doDetection: Found PATA ! \n");
                    kprintfd("IDEDriver::doDetection: port: %4X, drive: %d \n", base_port, cs%2);
                    
                    ATADriver *drv = new 
                    ATADriver( base_port, cs % 2, cs > 1 ? 14 : 13 );
                    
                    char *name = new char[5];
                    name[0] = 'A';
                    name[1] = 'T';
                    name[2] = 'A';
                    name[3] = cs + '0';
                    name[4] = '\0';
                    
                    // is seems that SWEB has problems with allocating
                    // it like this :
                    // char*name = "ATAX0";
                    
                    BDVirtualDevice *bdv = new 
                    BDVirtualDevice( drv, 0, drv->getNumSectors(),
                    drv->getSectorSize(), name, true);
                    
                    BDManager::getInstance()->addVirtualDevice( bdv );
                    
                    processMBR( drv, 0, drv->SPT );
                }
                else if( ( c4 == 0x3C ) && ( c5 == 0xC3 ) )
                {
                    kprintfd("IDEDriver::doDetection: Found SATA device! \n");
                    kprintfd("IDEDriver::doDetection: port: %4X, drive: %d \n", base_port, cs%2);
                  
                    // SATA hook 
                    // drv = new SATADriver ( base_port, cs % 2 );
                    
                    kprintfd("IDEDriver::doDetection: Running SATA device as PATA in compatibility mode! \n");
                    
                    char *name = new char[6];
                    name[0] = 'S';
                    name[1] = 'A';
                    name[2] = 'T';
                    name[3] = 'A';
                    name[4] = cs + '0';
                    name[5] = '\0';
                    
                    ATADriver *drv = new 
                    ATADriver( base_port, cs % 2, cs > 1 ? 14 : 13 );
                    
                    BDVirtualDevice *bdv = new 
                    BDVirtualDevice( drv, 0, drv->getNumSectors(),
                    drv->getSectorSize(), name, true);
                    
                    BDManager::getInstance()->addVirtualDevice( bdv );
                    
                    processMBR( drv, 0, drv->SPT );
                }
              }
              else
              {
                kprintfd("IDEDriver::doDetection: Unknown harddisk!\n");
              }
            }
              
          }
       }
    }
    else
    {
      kprintfd("IDEDriver::doDetection: Not found!\n ");
    }
  }

   // TODO : verify if the device is ATA and not ATAPI or SATA 
  return 0;
}

int32 IDEDriver::processMBR  ( ATADriver * drv, uint32 sector, uint32 SPT )
{
  uint32 offset = 0, numsec = 0;
  uint16 buff[256]; // read buffer
  kprintfd("IDEDriver::processMBR:reading MBR\n");
  
  BDRequest *br = new 
  BDRequest( 0, BDRequest::BD_READ, sector, 1, (void *) buff );
  
  drv->addRequest( br );                           // read the partition
  while( br->getStatus() == BDRequest::BD_QUEUED );// wait
  
  if( br->getStatus() == BDRequest::BD_ERROR )
  {
    kprintfd("IDEDriver::processMBR: drv returned BD_ERROR\n" );
    return -1;
  }

  MBR * mbr = (MBR *) buff;

  if( mbr->signature == 0xAA55 )
  {
    kprintfd("IDEDriver::processMBR: | Valid PC MBR | ");
    FP * fp = (FP *) mbr->parts;
    uint32 i;
    for(i = 0; i < 4; i++, fp++)
    {
      switch( fp->systid )
      {
      case 0x00:
        // do nothing
        break;
        case 0x05: // DOS extended partition
        case 0x0F: // Windows extended partition
        case 0x85: // linux extended partition
          kprintfd("ext. part. at: %d \n", fp->relsect );
          if ( processMBR( drv, sector + fp->relsect, SPT ) == -1 )
              processMBR( drv, sector + fp->relsect - SPT, SPT );
          break;
        default:
          // offset = fp->relsect - SPT;
          offset = fp->relsect;
          numsec = fp->numsect;
          kprintfd("part. offset : %d \n", offset);
                      
          BDVirtualDevice *bdv = new 
          BDVirtualDevice( drv, offset, numsec,
          drv->getSectorSize(), "PART", true);
          // TODO: setup names
          
          BDManager::getInstance()->addVirtualDevice( bdv );
        break;
      }
      }
    }
    else
    {
          kprintfd("IDEDriver::processMBR: | Invalid PC MBR %d | \n", mbr->signature);
        return -1;
    }

    kprintfd("IDEDriver::processMBR:, done with partitions \n");
    return 0;
}
