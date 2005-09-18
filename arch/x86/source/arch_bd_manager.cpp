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
  kprintfd("BDManager::doDeviceDetection:Detection done\n");
  // insert other device detectors here
}

void BDManager::addVirtualDevice( BDVirtualDevice* dev )
{
  kprintfd("BDManager::serviceIRQ:Adding device\n");
  device_list_.pushBack( dev );
  kprintfd("BDManager::serviceIRQ:Device added\n");
};

void BDManager::serviceIRQ( uint32 irq_num )
{
  kprintfd("BDManager::serviceIRQ:Servicing IRQ\n");
  probeIRQ = false;
  
  uint32 i = 0;
  for( i = 0; i < device_list_.size(); i++ )
    if( device_list_[i]->getDriver()->irq == irq_num )
      device_list_[i]->getDriver()->serviceIRQ( );
      
  kprintfd("BDManager::serviceIRQ:End servicing IRQ\n");
}

BDVirtualDevice* BDManager::getDeviceByNumber( uint32 dev_num  )
{
  return device_list_[ dev_num ];
};

BDVirtualDevice* BDManager::getDeviceByName( char * dev_name )
{
  return 0; // TODO: implement strcmp search
};

uint32 BDManager::getNumberOfDevices ( void )
{
  return device_list_.size();
};

BDManager* BDManager::instance_ = 0;
