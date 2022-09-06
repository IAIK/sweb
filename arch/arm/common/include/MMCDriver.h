#pragma once

#include "BDDriver.h"
#include "Mutex.h"

class BDRequest;

class MMCDriver : public BDDriver
{
  public:
    MMCDriver();
    virtual ~MMCDriver();

    /**
     * adds the given request to a list and checkes the type of the
     * request. If it is a read or write request, it is beeing executed,
     * or the function returns otherwise.
     *
     */
    uint32_t addRequest(BDRequest *);

    /**
     * @param 1 sector where it should be started to read
     * @param 2 number of sectors
     * @param 3 buffer where to save all that was read
     *
     */
    int32_t readSector(uint32_t, uint32_t, void *);

    /**
     * @param 1 sector where it should be started to write
     * @param 2 number of sectors
     * @param 3 buffer, which content should be written to the sectors
     *
     */
    int32_t writeSector(uint32_t, uint32_t, void *);

    uint32_t getNumSectors();
    uint32_t getSectorSize();
    void serviceIRQ();
    uint32_t SPT;
  private:

    /**
     * @param 1 start address
     * @param 2 buffer where to save the block that was read
     *
     */
    int32_t readBlock(uint32_t, void *);

    /**
     * @param 1 start address
     * @param 2 buffer, which content should be written to the bloc
     *
     */
    int32_t writeBlock(uint32_t, void *);
    Mutex lock_;

    uint32_t rca_;
    uint32_t sector_size_;
    uint32_t num_sectors_;
};
