/**
 * Filename: FsDeviceFile.h
 * Description:
 *
 * Created on: 13.05.2012
 * Author: chris
 */

#ifdef USE_FILE_SYSTEM_ON_GUEST_OS

#ifndef FSDEVICEFILE_H_
#define FSDEVICEFILE_H_

#include "FsDevice.h"

#include <stdio.h>

/**
 * @class special FsDevice, redirecting all data to File-System calls on the
 * guest os (used to create SWEB-kernel images)
 */
class FsDeviceFile : public FsDevice
{
  public:
    /**
     * constructor
     * @param image_file the image-file emulating a full-device
     * @param offset the offset of the partition to use in bytes
     * @param part_size the size of the partition in use in bytes
     * @param block_size the initially used block-size
     */
    FsDeviceFile(const char* image_file, sector_addr_t offset,
                 sector_addr_t part_size, sector_len_t block_size = SECTOR_SIZE);

    /**
     * destructor
     */
    virtual ~FsDeviceFile();

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
    virtual sector_len_t getBlockSize(void) const;

    /**
     * getting the number of Blocks (= the Size of the Device)
     * @return number of available blocks
     */
    virtual sector_addr_t getNumBlocks(void) const;

  private:

    // the image file to write / read to / from
    int img_fd_;
    //FILE* image_file_;

    // the beginning of the partition in bytes from the start of the file
    uint64 partition_offset_;

    // the length of the partition in bytes
    uint64 partition_len_;

    // the simulated sector-size
    static const sector_len_t SECTOR_SIZE = 512;

    // user defined block-size
    sector_len_t block_size_;

    // number of blocks at current block_size_
    sector_addr_t num_blocks_;

    // the size of the image in bytes
    uint64 image_size_;
};

#endif /* FSDEVICEFILE_H_ */
#endif
