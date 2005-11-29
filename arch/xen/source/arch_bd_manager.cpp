/********************************************************************
*
*    $Id: arch_bd_manager.cpp,v 1.2 2005/11/29 15:14:16 rotho Exp $
*    $Log: arch_bd_manager.cpp,v $
*    Revision 1.1  2005/09/21 02:18:58  rotho
*    temporary commit; still doesn't work
*
********************************************************************/

#include "arch_bd_manager.h"

BDManager * BDManager::getInstance()
{
  if( !instance_ )
    instance_ = new BDManager();
    
  return instance_;
};

BDManager::BDManager(){}

void BDManager::doDeviceDetection( void ){}

void BDManager::addRequest( BDRequest* bdr ){}

void BDManager::addVirtualDevice( BDVirtualDevice* dev ){}

void BDManager::serviceIRQ( uint32 irq_num ){}

BDVirtualDevice* BDManager::getDeviceByNumber( uint32 dev_num  ){}

BDVirtualDevice* BDManager::getDeviceByName( char * dev_name ){return 0;}

uint32 BDManager::getNumberOfDevices ( void ){}

BDManager* BDManager::instance_ = 0;
