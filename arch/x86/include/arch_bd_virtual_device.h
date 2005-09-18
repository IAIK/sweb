#ifndef _BD_VIRTUAL_DEVICE_
#define _BD_VIRTUAL_DEVICE_

#include "types.h"
#include "string.h"

#include "arch_bd_request.h"
#include "arch_bd_driver.h"

class BDVirtualDevice
{
  public:
    
    
    BDVirtualDevice( BDDriver * driver, uint32 offset, uint32 num_blocks, uint32 block_size, char *name, bool writable);
  
    void BDVirtualDevice::addRequest(BDRequest * command);
    
    uint32    getBlockSize()                  { return block_size_; };
    uint32    getDeviceNumber()               { return dev_number_; };
    BDDriver* getDriver()                     { return driver_; };
    char*     getName()                       { return name_; };
    uint32    getNumBlocks()                  { return num_blocks_; };
    
    
    
    void      setDeviceNumber( uint32 number ) { dev_number_ = number; };
    
     
  private:
    BDVirtualDevice();
    
    uint32 dev_number_;
    uint32 block_size_;
    uint32 num_blocks_;
    uint32 offset_;
    bool writable_;
    char *name_;
    BDDriver * driver_;
};

#endif
