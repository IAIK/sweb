/**
 * Filename: FsDevice.h
 * Description:
 *
 * Created on: 13.05.2012
 * Author: chris
 */

#ifndef FSDEVICE_H_
#define FSDEVICE_H_

#include "fs/FsDefinitions.h"
#include "cache/GeneralCache.h"

/**
 * @class adapter class for Devices used with the FileSystem
 * possible implementations are BlockDevices, Files on a guest-OS (for
 * image creation at compilation-time) and for forth
 *
 * furthermore the FsDevice implements the Cache::DeviceAdapter in order
 * to be used within a Device-Cache
 */
class FsDevice : public Cache::DeviceAdapter
{
  public:
    FsDevice();
    virtual ~FsDevice();

    /**
     * reads a Sector from the underlining (Block)Device
     * @param sector
     * @param buffer
     * @param buffer_size has to be n-times the current Device BLOCK_SIZE, where
     * n is a positive not 0 integer
     * @return true / false
     */
    virtual bool readSector(sector_addr_t sector, char* buffer, sector_len_t buffer_size) = 0;

    /**
     * writes a Sector to the underlining (Block)Device
     * @param sector
     * @param buffer
     * @param buffer_size
     * @return true / false
     */
    virtual bool writeSector(sector_addr_t sector, const char* buffer, sector_len_t buffer_size) = 0;

    /**
     * applies a new BlockSize to the FsDevice
     *
     * @param new_block_size the new BlockSize (has to be mod 512 = 0)
     */
    virtual void setBlockSize(sector_len_t new_block_size) = 0;

    /**
     * getting the current Block-Size
     * @return the current Block-Size
     */
    virtual sector_len_t getBlockSize(void) const = 0;

    /**
     * getting the number of Blocks (= the Size of the Device)
     * @return number of available blocks
     */
    virtual sector_addr_t getNumBlocks(void) const = 0;

    /**
     * IMPLEMENTS the DeviceAdapter read() method, this is just another
     * version of the readSector() method
     *
     * for details see GeneralCache.h
     */
    virtual Cache::Item* read(const Cache::ItemIdentity& ident);

    /**
     * IMPLEMENTS the DeviceAdapter write() method, this is just another
     * version of the writeSector() method
     */
    virtual bool write(const Cache::ItemIdentity& ident, Cache::Item* data);

    /**
     * IMPLEMENTS the DeviceAdapter remove() method, this is empty
     * due to the fact that a sector can not be removed from a Device
     */
    virtual bool remove(const Cache::ItemIdentity& ident, Cache::Item* item = NULL);

};

#endif /* FSDEVICE_H_ */
