#pragma once

#include "BDDriver.h"
#include <cstddef>

class BDRequest;
class BDVirtualDevice;

class RamDiskDriver : public BDDriver
{
public:
    RamDiskDriver(void* start_vaddr, size_t size);
    ~RamDiskDriver() override;

    uint32_t addRequest(BDRequest*) override;

    int32_t readSector(uint32_t start_sector, uint32_t num_sectors, void* buffer) override;

    int32_t writeSector(uint32_t start_sector, uint32_t num_sectors, void* buffer) override;

    uint32_t getNumSectors() override;

    uint32_t getSectorSize() override;

    void serviceIRQ() override;

    static BDVirtualDevice* createRamDisk(void* start_vaddr, size_t size, const char* name);

private:
    void* start_vaddr_;
    size_t size_;
};
