/**
 * @file DeviceFSSuperBlock.h
 */

#ifndef DEVICEFS_SUPERBLOCK_H__
#define DEVICEFS_SUPERBLOCK_H__

#include "fs/Superblock.h"

class Inode;
class Superblock;
class CharacterDevice;

/**
 * @class DeviceFSSuperBlock
 */
class DeviceFSSuperBlock : public Superblock
{
  public:
    static const char ROOT_NAME[];
    static const char DEVICE_ROOT_NAME[];

    /**
     * Destructor
     */
    virtual ~DeviceFSSuperBlock();

    /**
     * creates a new Inode of the superblock
     * @param dentry the dentry for the new indoe
     * @param type the type of the new inode
     * @return the inode
     */
    virtual Inode* createInode(Dentry* dentry, uint32 type);

    /**
     * creates a file descriptor for the given inode
     * @param inode the inode to create the fd for
     * @param flag the flag of the fd
     * @return the file descriptor
     */
    virtual int32 createFd(Inode* inode, uint32 flag);

    /**
     * addsa new device to the superblock
     * @param inode the inode of the device to add
     * @param device_name the device name
     */
    void addDevice(Inode* inode, const char* device_name);

    /**
     * Access method to the singleton instance
     */
    static DeviceFSSuperBlock* getInstance()
    {
      if (!instance_)
        instance_ = new DeviceFSSuperBlock(0, 0);
      return instance_;
    }

  private:

    /**
     * Constructor
     * @param s_root the root Dentry of the new Filesystem
     * @param s_dev the device number of the new Filesystem
     */
    DeviceFSSuperBlock(Dentry* s_root, uint32 s_dev);

    Inode* cDevice;
    Dentry* s_dev_dentry_;

  protected:
    static DeviceFSSuperBlock* instance_;

};

#endif
