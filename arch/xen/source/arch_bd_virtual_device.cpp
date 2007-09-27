/********************************************************************
*
*    $Id: arch_bd_virtual_device.cpp,v 1.2 2005/11/29 15:14:16 rotho Exp $
*    $Log: arch_bd_virtual_device.cpp,v $
*    Revision 1.1  2005/09/21 03:33:52  rotho
*    compiles now, but still doesn't work
*
********************************************************************/

#include "arch_bd_virtual_device.h"

BDVirtualDevice::BDVirtualDevice(BDDriver * driver, uint32 offset, uint32 num_blocks, uint32 block_size, char *name, bool writable)
{
};

void BDVirtualDevice::addRequest(BDRequest * command) 
{
  return;
};

