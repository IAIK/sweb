/********************************************************************
*
*    $Id: arch_bd_manager.cpp,v 1.7 2006/09/19 20:40:23 aniederl Exp $
*    $Log: arch_bd_manager.cpp,v $
*    Revision 1.6  2005/11/24 23:38:35  nelles
*
*
*     Block devices fix.
*
*     Committing in .
*
*     Modified Files:
*     	arch_bd_ata_driver.cpp arch_bd_ide_driver.cpp
*     	arch_bd_manager.cpp arch_bd_virtual_device.cpp
*
*    Revision 1.5  2005/10/02 12:27:55  nelles
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
*    Revision 1.4  2005/09/26 15:29:05  btittelbach
*    check
*
*    Revision 1.3  2005/09/18 20:46:52  nelles
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

#include "arch_bd_manager.h"
#include "arch_bd_ide_driver.h"

#include "kprintf.h"

BDManager * BDManager::getInstance()
{
  if( !instance_ )
    instance_ = new BDManager();
    
  return instance_;
};

BDManager::BDManager()
{
}

void BDManager::doDeviceDetection( void )
{
  kprintfd("BDManager::doDeviceDetection:Detecting IDE devices\n");
  IDEDriver id;
    // insert other device detectors here
  kprintfd("BDManager::doDeviceDetection:Detection done\n");
}

void BDManager::addRequest( BDRequest* bdr )
{
  if( bdr->getDevID() < getNumberOfDevices() )
    getDeviceByNumber( bdr->getDevID() )->addRequest( bdr );
  else
    bdr->setStatus( BDRequest::BD_ERROR );
}

void BDManager::addVirtualDevice( BDVirtualDevice* dev )
{
  kprintfd("BDManager::addVirtualDevice:Adding device\n");
  device_list_.pushBack( dev );
  kprintfd("BDManager::addVirtualDevice:Device added\n");
};

void BDManager::serviceIRQ( uint32 irq_num )
{
  kprintfd("BDManager::serviceIRQ:Servicing IRQ\n");
  probeIRQ = false;
  
  uint32 i = 0;
  for( i = 0; i < device_list_.size(); i++ )
    if( device_list_[i]->getDriver()->irq == irq_num )
	{	
      device_list_[i]->getDriver()->serviceIRQ( );
	  return;
	}
      
  kprintfd("BDManager::serviceIRQ:End servicing IRQ\n");
}

BDVirtualDevice* BDManager::getDeviceByNumber( uint32 dev_num  )
{
  return device_list_[ dev_num ];
};

// BDVirtualDevice* BDManager::getDeviceByName( char * dev_name )
// {
// #warning TODO: implement strcmp search for getDeviceByName
//   return 0; // TODO: implement strcmp search
// };

uint32 BDManager::getNumberOfDevices ( void )
{
  return device_list_.size();
};

BDManager* BDManager::instance_ = 0;
