/**
 * Filename: FsDeviceVirtual.h
 * Description:
 *
 * Created on: 19.05.2012
 * Author: chris
 */

#ifndef FSDEVICEVIRTUAL_H_
#define FSDEVICEVIRTUAL_H_

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
#include "FsDevice.h"

// forwards:
class BDVirtualDevice;

/**
 * @class Adapter class for BDVirtualDevice to be used as a FsDevice
 */
class FsDeviceVirtual : public FsDevice
{
  public:
    /**
     * constructor
     * @param device the BDVirtualDevice to wrapp around
     */
    FsDeviceVirtual(BDVirtualDevice* device);

    virtual ~FsDeviceVirtual();

    /**
     * reads a Sector from the underlining (Block)Device
     * @param sector
     * @param buffer
     * @param buffer_size has to be n-times the current Device BLOCK_SIZE, where
     * n is a positive not 0 integer
     * @return true / false
     */
    virtual bool readSector(sector_addr_t sector, char* buffer, sector_len_t buffer_size);

    /**
     * writes a Sector to the underlining (Block)Device
     * @param sector
     * @param buffer
     * @param buffer_size
     * @return true / false
     */
    virtual bool writeSector(sector_addr_t sector, const char* buffer, sector_len_t buffer_size);

    /**
     * applies a new BlockSize to the FsDevice
     *
     * @param new_block_size the new BlockSize (has to be mod 512 = 0)
     */
    virtual void setBlockSize(sector_len_t new_block_size);

    /**
     * getting the current Block-Size
     * @return the current Block-Size
     */
    virtual sector_len_t getBlockSize() const;

    /**
     * getting the number of Blocks (= the Size of the Device)
     * @return number of available blocks
     */
    virtual sector_addr_t getNumBlocks(void) const;

  private:

    // the wrapped device
    BDVirtualDevice* dev_;
};

#endif // USE_FILE_SYSTEM_ON_GUEST_OS

#endif /* FSDEVICEVIRTUAL_H_ */
