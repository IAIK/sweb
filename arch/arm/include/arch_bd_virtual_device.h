/**
 * @file arch_bd_virtual_device.h
 *
 */

#ifndef _BD_VIRTUAL_DEVICE_
#define _BD_VIRTUAL_DEVICE_

#include "types.h"

#include "ustl/ulist.h"
#include "Mutex.h"

class BDDriver;
class BDRequest;

class BDDriver;
class BDRequest;

class BDVirtualDevice
{
  public:

    /**
     * Constructor
     *
     */
    BDVirtualDevice( BDDriver *driver, uint32 offset, uint32 num_sectors, uint32 sector_size, char *name, bool writable);

    /**
     * adds the given request to the device given in the request
     * @param command the request
     *
     */
    void addRequest(BDRequest *command);

    /**
     * @return returns the size of one block
     * now 1024
     *
     */
    uint32 getBlockSize() const { return block_size_; };

    /**
     * @return returns the current device number
     *
     */
    uint32 getDeviceNumber() const { return dev_number_; };

    /**
     * @return returns the current driver
     *
     */
    BDDriver *getDriver() { return driver_; };

    /**
     * @return returns the current name
     *
     */
    char *getName() { return name_; };

    /**
     * calculates the number of blocks
     *
     */
    uint32 getNumBlocks() { return num_sectors_ / (block_size_/sector_size_); };

    /**
     * reads the data from the inode on the current device
     * @param offset where to start to read
     * @param size number of bytes that should be read
     * @param buffer to save the data that has been read
     *
     */
    virtual int32 readData(uint32 offset, uint32 size, char *buffer);

    /**
     * reads the data from the inode on the current device
     * @param offset where to start to write
     * @param size number of bytes that should be written
     * @param buffer data, that should be written
     *
     */
    virtual int32 writeData(uint32 offset, uint32 size, char *buffer);

    /**
     * the PartitionType is a 8bit field in the PartitionTable of a MBR
     * it specifies the FileSystem which is installed on the partition
     * @param part_type partition type value to be applied to the Device
     */
    void setPartitionType(uint8 part_type);

    /**
     * getting the PartitionType of this Device (value of the 8bit field
     * in Partition Table of the MBR)
     * @return the partition type
     */
    uint8 getPartitionType(void) const;

    /**
     * sets the device number
     *
     */
    void setDeviceNumber( uint32 number ) { dev_number_ = number; };

    /**
     * sets the blocksize
     *
     */
    void setBlockSize( uint32 block_size )
    {
      assert(block_size % sector_size_ == 0);
      block_size_ = block_size;
    };


  private:

    /**
     * private Constuctor, should not be used!
     *
     */
    BDVirtualDevice();
    uint32 dev_number_;
    uint32 block_size_;
    uint32 sector_size_;
    uint32 num_sectors_;
    uint32 offset_;
    bool writable_;
    char *name_;
    BDDriver * driver_;
    uint8 partition_type_;
};

#endif
