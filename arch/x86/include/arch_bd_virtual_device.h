/**
 * @file arch_bd_virtual_device.h
 *
 */

#ifndef _BD_VIRTUAL_DEVICE_
#define _BD_VIRTUAL_DEVICE_

#include "types.h"
#include "arch_bd_request.h"
#include "arch_bd_driver.h"

#include "fs/PointList.h"
#include "fs/Inode.h"
#include "fs/ramfs/RamFSFile.h"
#include "fs/devicefs/DeviceFSSuperblock.h"
#include "fs/Dentry.h"
#include "fs/Superblock.h"

class BDVirtualDevice : public Inode
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
    uint32 getBlockSize() { return block_size_; };

    /**
     * @return returns the current device number
     *
     */
    uint32 getDeviceNumber() { return dev_number_; };

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
    uint32 getNumBlocks() { return num_sectors_ * block_size_ / sector_size_; };


//// Inode functions
    /**
     * reads the data from the inode on the current device
     * @param offset where to start to read
     * @param size number of bytes that should be read
     * @param buffer to save the data that has been read
     *
     */
    virtual int32 readData(int32 offset, int32 size, char *buffer);

    /**
     * reads the data from the inode on the current device
     * @param offset where to start to write
     * @param size number of bytes that should be written
     * @param buffer data, that should be written
     *
     */
    virtual int32 writeData(int32 offset, int32 size, const char *buffer);

    /**
     * creates an inode at the given dentry
     *
     */
    int32 mknod(Dentry *dentry)
    {
      if(dentry == 0)
        return -1;

      i_dentry_ = dentry;
      dentry->setInode(this);
      return 0;
    }

    /**
     * creates an inode at the given dentry
     *
     */
    int32 create(Dentry *dentry)
    {
      return(mknod(dentry));
    }

    /**
     * creates a file at the given dentry
     *
     */
    int32 mkfile(Dentry *dentry)
    {
      return(mknod(dentry));
    }

    /**
     * creates a link to the current file
     *
     */
    File *link(uint32 flag)
    {
      File* file = (File*)(new RamFSFile(this, i_dentry_, flag));
      i_files_.pushBack(file);
      return file;
    }

    /**
     * deletes the link to the given file
     *
     */
    int32 unlink(File* file)
    {
      int32 tmp = i_files_.remove(file);
      delete file;
      return tmp;
    }
//// End Inode functions

    /**
     * sets the device number
     *
     */
    void setDeviceNumber( uint32 number ) { dev_number_ = number; };

    /**
     * sets the blocksize
     *
     */
    void setBlockSize( uint32 block_size ) { block_size_ = block_size; };


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
};

#endif
