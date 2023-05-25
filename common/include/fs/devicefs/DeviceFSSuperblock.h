#pragma once

#include "fs/Superblock.h"
#include "fs/ramfs/RamFSSuperblock.h"

class Inode;
class Superblock;
class CharacterDevice;
class DeviceFSType;
class Device;

class DeviceFSSuperBlock : public RamFSSuperblock
{
  public:
    static const char ROOT_NAME[];
    static const char DEVICE_ROOT_NAME[];

    ~DeviceFSSuperBlock() override;

    /**
     * addsa new device to the superblock
     * @param device_inode the inode of the device to add
     * @param device_name the device name
     */
    void addDevice(Inode* device_inode, const char* device_name);

    /**
     * Access method to the singleton instance
     */
    static DeviceFSSuperBlock* getInstance();

    void addBlockDeviceInodes();

    void addDeviceInodes(Device& device_root);

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
