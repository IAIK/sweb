#pragma once

#include "BDDriver.h"
#include "Device.h"
#include "DeviceDriver.h"
#include "Mutex.h"

class BDRequest;

class MMCDrive : public BDDriver, public Device
{
  public:
    NO_OPTIMIZE MMCDrive();
    ~MMCDrive() override;

    /**
     * adds the given request to a list and checkes the type of the
     * request. If it is a read or write request, it is beeing executed,
     * or the function returns otherwise.
     *
     */
    uint32 addRequest(BDRequest *) override;

    /**
     * @param 1 sector where it should be started to read
     * @param 2 number of sectors
     * @param 3 buffer where to save all that was read
     *
     */
    int32 readSector(uint32, uint32, void *) override;

    /**
     * @param 1 sector where it should be started to write
     * @param 2 number of sectors
     * @param 3 buffer, which content should be written to the sectors
     *
     */
    int32 writeSector(uint32, uint32, void *) override;

    uint32 getNumSectors() override;
    uint32 getSectorSize() override;
    void serviceIRQ() override;
    uint32 SPT;
  private:

    /**
     * @param 1 start address
     * @param 2 buffer where to save the block that was read
     *
     */
    int32 readBlock(uint32, void *);

    /**
     * @param 1 start address
     * @param 2 buffer, which content should be written to the bloc
     *
     */
    int32 writeBlock(uint32, void *);
    Mutex lock_;

    uint32 rca_;
    uint32 sector_size_;
    uint32 num_sectors_;
};

class MMCDeviceDriver : public BasicDeviceDriver,
                        public Driver<MMCDrive>
{
public:
    MMCDeviceDriver();
    ~MMCDeviceDriver() override = default;

    static MMCDeviceDriver& instance();

    void doDeviceDetection() override;

private:
};
