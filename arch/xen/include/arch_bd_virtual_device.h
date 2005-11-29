/********************************************************************
 *
 *    $Id: arch_bd_virtual_device.h,v 1.2 2005/11/29 15:14:16 rotho Exp $
 *    $Log: arch_bd_virtual_device.h,v $
 *    Revision 1.1  2005/09/20 16:30:31  rotho
 *    Block-Device headers copied from arch/x86/include/
 *
 ********************************************************************/

#ifndef _BD_VIRTUAL_DEVICE_
#define _BD_VIRTUAL_DEVICE_

#include "types.h"
#include "arch_bd_request.h"

class BDDriver{};

class BDVirtualDevice
{
public:
    BDVirtualDevice( BDDriver * driver, uint32 offset, uint32 num_blocks, uint32 block_size, char *name, bool writable);
    void BDVirtualDevice::addRequest(BDRequest * command);
    
    uint32    getBlockSize()                  { return 0; };
    uint32    getDeviceNumber()               { return 0; };
    char*     getName()                       { return name_; };
    uint32    getNumBlocks()                  { return 0; };
    char *name_;
};

#endif
