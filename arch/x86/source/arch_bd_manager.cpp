
#include "arch_bd_manager.h"
#include "arch_bd_ide_driver.h"

#include "kprintf.h"
#include "debug.h"
#include "string.h"

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
  debug(BD_MANAGER, "doDeviceDetection: Detecting IDE devices\n");
  IDEDriver id;
    // insert other device detectors here
  debug(BD_MANAGER, "doDeviceDetection:Detection done\n");
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
  debug(BD_MANAGER, "addVirtualDevice:Adding device\n");
  dev->setDeviceNumber(device_list_.size());
  device_list_.pushBack( dev );
  debug(BD_MANAGER, "addVirtualDevice:Device added\n");
}

void BDManager::serviceIRQ( uint32 irq_num )
{
  debug(BD_MANAGER, "serviceIRQ:Servicing IRQ\n");
  probeIRQ = false;
  
  uint32 i = 0;
  for( i = 0; i < device_list_.size(); i++ )
    if( device_list_[i]->getDriver()->irq == irq_num )
	{	
      device_list_[i]->getDriver()->serviceIRQ( );
	  return;
	}
      
  debug(BD_MANAGER, "serviceIRQ:End servicing IRQ\n");
}

BDVirtualDevice* BDManager::getDeviceByNumber( uint32 dev_num  )
{
  return device_list_[ dev_num ];
}

BDVirtualDevice* BDManager::getDeviceByName( const char * dev_name )
{
  debug(BD_MANAGER, "getDeviceByName: %s", dev_name);
  for ( uint32 index = 0; index < device_list_.size(); index++)
  {
    if(strcmp(device_list_[ index ]->getName(),dev_name) == 0)
    {
      debug(BD_MANAGER, "getDeviceByName%d: %s with id: %d",index, device_list_[ index ]->getName(), device_list_[ index ]->getDeviceNumber());
      return device_list_[ index ];
    }
  }
  return 0;
}

uint32 BDManager::getNumberOfDevices ( void )
{
  return device_list_.size();
}

BDManager* BDManager::instance_ = 0;
