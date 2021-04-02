#pragma once

#include "fs/Superblock.h"

class Inode;
class Superblock;
class CharacterDevice;
class DeviceFSType;

class DeviceFSSuperBlock : public Superblock
{
  public:
    static const char ROOT_NAME[];
    static const char DEVICE_ROOT_NAME[];

    virtual ~DeviceFSSuperBlock();

    /**
     * creates a new Inode of the superblock
     * @param dentry the dentry for the new indoe
     * @param type the type of the new inode
     * @return the inode
     */
    virtual Inode* createInode(uint32 type);

    /**
     * addsa new device to the superblock
     * @param inode the inode of the device to add
     * @param node_name the device name
     */
    void addDevice(Inode* inode, const char* node_name);

    /**
     * Access method to the singleton instance
     */
    static DeviceFSSuperBlock* getInstance();

  private:

    /**
     * Constructor
     * @param s_root the root Dentry of the new Filesystem
     * @param s_dev the device number of the new Filesystem
     */
    DeviceFSSuperBlock(DeviceFSType* fs_type, uint32 s_dev);

  protected:
    static DeviceFSSuperBlock* instance_;

};

