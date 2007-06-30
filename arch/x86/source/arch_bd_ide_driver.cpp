/********************************************************************
*
*    $Id: arch_bd_ide_driver.cpp,v 1.7 2005/11/27 11:57:06 woswasi Exp $
*    $Log: arch_bd_ide_driver.cpp,v $
*    Revision 1.5  2005/11/24 23:38:35  nelles
*     Block devices fix.
*
*     Committing in .
*
*     Modified Files:
*     	arch_bd_ata_driver.cpp arch_bd_ide_driver.cpp
*     	arch_bd_manager.cpp arch_bd_virtual_device.cpp
*
*    Revision 1.4  2005/10/24 21:28:04  nelles
*
*     Fixed block devices. I think.
*
*     Committing in .
*
*     Modified Files:
*
*     	arch/x86/include/arch_bd_ata_driver.h
*     	arch/x86/source/InterruptUtils.cpp
*     	arch/x86/source/arch_bd_ata_driver.cpp
*     	arch/x86/source/arch_bd_ide_driver.cpp
*     	arch/xen/source/arch_bd_ide_driver.cpp
*
*     	common/source/kernel/SpinLock.cpp
*     	common/source/kernel/Thread.cpp utils/bochs/bochsrc
*
*    Revision 1.3  2005/10/02 12:27:55  nelles
*
*     Committing in .
*
*    	DeviceFS patch. The devices can now be accessed through VFS.
*
*
*
*
*     Modified Files:
*     	Makefile arch/x86/include/arch_bd_ata_driver.h
*     	arch/x86/include/arch_bd_driver.h
*     	arch/x86/include/arch_bd_ide_driver.h
*     	arch/x86/include/arch_bd_virtual_device.h
*     	arch/x86/source/InterruptUtils.cpp arch/x86/source/Makefile
*     	arch/x86/source/arch_bd_ide_driver.cpp
*     	arch/x86/source/arch_bd_manager.cpp
*     	arch/x86/source/arch_bd_virtual_device.cpp
*     	arch/x86/source/arch_serial.cpp
*     	arch/x86/source/arch_serial_manager.cpp
*     	common/include/console/Terminal.h
*     	common/include/drivers/serial.h common/include/fs/Inode.h
*     	common/include/fs/Superblock.h common/include/fs/fs_global.h
*     	common/include/kernel/TestingThreads.h
*     	common/source/console/FrameBufferConsole.cpp
*     	common/source/console/Makefile
*     	common/source/console/Terminal.cpp
*     	common/source/console/TextConsole.cpp
*     	common/source/fs/Dentry.cpp common/source/fs/Makefile
*     	common/source/fs/PathWalker.cpp
*     	common/source/fs/Superblock.cpp
*     	common/source/fs/VfsSyscall.cpp common/source/kernel/main.cpp
*     	utils/bochs/bochsrc
*     Added Files:
*     	common/include/drivers/chardev.h
*     	common/include/fs/devicefs/DeviceFSSuperblock.h
*     	common/include/fs/devicefs/DeviceFSType.h
*     	common/source/fs/devicefs/DeviceFSSuperblock.cpp
*     	common/source/fs/devicefs/DeviceFSType.cpp
*     	common/source/fs/devicefs/Makefile
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

#include "arch_bd_ide_driver.h"
#include "arch_bd_ata_driver.h"
#include "arch_bd_virtual_device.h"
#include "arch_bd_manager.h"
#include "ports.h"
#include "kmalloc.h"
#include "string.h"

uint32 IDEDriver::doDeviceDetection()
{
   uint32 jiffies = 0;
   uint16 base_port = 0x1F0;
   uint16 base_regport = 0x3F6; 
   uint8 cs = 0;
   uint8 sc;
   uint8 sn;
   uint8 devCtrl;
	
   uint8 ata_irqs[4] = { 14, 15, 11, 9 };

   // setup register values

   devCtrl = 0x00; // please use interrupts

   // assume there are no devices

   kprintfd("IDEDriver::doDetection:%d\n", cs);   
    
   for( cs = 0; cs < 4; cs ++)
   {
      char *name = new char[5];
      name[0] = 'i';
      name[1] = 'd';
      name[2] = 'e';
      name[3] = cs + 'a';
      name[4] = '\0';  
      
      kprintfd("IDEDriver::doDetection:Detecting IDE DEV: %s\n", name); 

      if( cs > 1 )
      {
        base_port = 0x170;
        base_regport = 0x376;
      }

      outbp( base_regport, devCtrl ); // init the device with interupts

      uint8 value = (cs % 2 == 0 ? 0xA0 : 0xB0 );
      uint16 bpp6 = base_port + 6;
      uint16 bpp2 = base_port + 2;
      uint16 bpp3 = base_port + 3;
      
      outportb( bpp6, value );  
      outportb( 0x80, 0x00 );

      outportb( bpp2, 0x55 );
      outportb( 0x80, 0x00 );
      outportb( bpp3, 0xAA );
      outportb( 0x80, 0x00 );
      outportb( bpp2, 0xAA );
      outportb( 0x80, 0x00 );
      outportb( bpp3, 0x55 );
      outportb( 0x80, 0x00 );
      outportb( bpp2, 0x55 );
      outportb( 0x80, 0x00 );
      outportb( bpp3, 0xAA );
      outportb( 0x80, 0x00 );

      sc = inportb( bpp2 );
      outportb( 0x80, 0x00 );
      sn = inportb( bpp3 );
      outportb( 0x80, 0x00 );
      
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
                    
                    ATADriver *drv = new ATADriver( base_port, cs % 2, ata_irqs[cs] );
                    kprintfd("IDEDriver::doDetection: 1\n");
                    
                    BDVirtualDevice *bdv = new BDVirtualDevice( drv, 0, drv->getNumSectors(),
                      drv->getSectorSize(), name, true);
                    kprintfd("IDEDriver::doDetection: 2\n");
                    
                    BDManager::getInstance()->addVirtualDevice( bdv );
                    kprintfd("IDEDriver::doDetection: 3\n");
                    
                    processMBR( drv, 0, drv->SPT, name );

                    kprintfd("IDEDriver::doDetection: 4\n");
                }
                else if( ( c4 == 0x3C ) && ( c5 == 0xC3 ) )
                {
                    kprintfd("IDEDriver::doDetection: Found SATA device! \n");
                    kprintfd("IDEDriver::doDetection: port: %4X, drive: %d \n", base_port, cs%2);
                  
                    // SATA hook 
                    // drv = new SATADriver ( base_port, cs % 2 );
                    
                    kprintfd("IDEDriver::doDetection: Running SATA device as PATA in compatibility mode! \n");
                    
                    ATADriver *drv = new 
                    ATADriver( base_port, cs % 2, ata_irqs[cs] );
                    
                    BDVirtualDevice *bdv = new 
                    BDVirtualDevice( drv, 0, drv->getNumSectors(),
                                     drv->getSectorSize(), name, true);
                    
                    BDManager::getInstance()->addVirtualDevice( bdv );
                    
                    processMBR( drv, 0, drv->SPT, name );
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
    
    delete name;
  }

   // TODO : verify if the device is ATA and not ATAPI or SATA 
  return 0;
}

int32 IDEDriver::processMBR  ( ATADriver * drv, uint32 sector, uint32 SPT, char *name )
{
  uint32 offset = 0, numsec = 0;
  uint16 buff[256]; // read buffer
  kprintfd("IDEDriver::processMBR:reading MBR\n");
  
  static uint32 part_num = 0;
//   char part_num_str[2];
//   char part_name[10];

  uint32 read_res = drv->rawReadSector( sector, 1, (void *)buff );
  
  if( read_res != 0 )
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
          if ( processMBR( drv, sector + fp->relsect, SPT, name) == -1 )
              processMBR( drv, sector + fp->relsect - SPT, SPT, name );
          break;
        default:
          // offset = fp->relsect - SPT;
          offset = fp->relsect;
          numsec = fp->numsect;
//           kprintfd("part. offset : %d \n", offset);
// 
//           part_num_str[0] = part_num + '0';
//           part_num_str[1] = 0;
//         
//           kprintfd("strlen name : %d \n", strlen(name) );
//           strncpy( part_name, strncat( name, part_num_str, 1 ), strlen(name) + 2);
//           kprintfd("Copied\n" );
//           kprintfd("now: %s\n", part_name );
//           kprintfd("Now creating\n" );

          char *part_name = (char*) kmalloc( 6 );
          strncpy( part_name, name, 4 );
          part_name[4] = part_num + '0';
          part_name[5] = 0;
          part_num++;          
          BDVirtualDevice *bdv = new 
          BDVirtualDevice( drv, offset, numsec,
          drv->getSectorSize(), part_name, true);
          kprintfd("Created\n" );
          BDManager::getInstance()->addVirtualDevice( bdv );
          kprintfd("Added\n" );
          kfree( part_name );
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
