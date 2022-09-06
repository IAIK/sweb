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

    virtual uint32_t addRequest(BDRequest *);

    virtual int32_t readSector( uint32_t start_sector, uint32_t num_sectors, void *buffer );

    virtual int32_t writeSector( uint32_t start_sector, uint32_t num_sectors, void * buffer );

    virtual uint32_t getNumSectors();

    virtual uint32_t getSectorSize();

    virtual void serviceIRQ();

    static BDVirtualDevice* createRamDisk(void* start_vaddr, size_t size, const char* name);

  private:
    void* start_vaddr_;
    size_t size_;
};
