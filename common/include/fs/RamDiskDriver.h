#pragma once

#include "BDDriver.h"
#include <cstddef>

class BDRequest;
class BDVirtualDevice;

class RamDiskDriver : public BDDriver
{
  public:
    RamDiskDriver(void* start_vaddr, size_t size);
    virtual ~RamDiskDriver();

    virtual uint32 addRequest(BDRequest *);

    virtual int32 readSector( uint32 start_sector, uint32 num_sectors, void *buffer );

    virtual int32 writeSector( uint32 start_sector, uint32 num_sectors, void * buffer );

    virtual uint32 getNumSectors();

    virtual uint32 getSectorSize();

    virtual void serviceIRQ();

    static BDVirtualDevice* createRamDisk(void* start_vaddr, size_t size, const char* name);

  private:
    void* start_vaddr_;
    size_t size_;
};
