#pragma once

#include <cstdint>

class BDRequest;

class BDDriver
{
  public:
    BDDriver() = default;

    BDDriver(uint16_t irq) :
        irq(irq)
    {
    }

    virtual ~BDDriver() = default;

    virtual uint32_t addRequest(BDRequest *) = 0;

    virtual int32_t readSector(uint32_t, uint32_t, void *) = 0;

    virtual int32_t writeSector(uint32_t, uint32_t, void *) = 0;

    virtual uint32_t getNumSectors() = 0;

    virtual uint32_t getSectorSize() = 0;

    virtual void serviceIRQ() = 0;

    uint16_t irq;
};
