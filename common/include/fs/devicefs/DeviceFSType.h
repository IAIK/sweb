#pragma once

#include "fs/FileSystemType.h"
#include "fs/ramfs/RamFSType.h"

class DeviceFSType : public RamFSType
{
public:
    DeviceFSType();

    ~DeviceFSType() override = default;

    /**
     * Reads the superblock from the device.
     * @param superblock is the superblock to fill with data.
     * @param data is the data given to the mount system call.
     * @return is a pointer to the resulting superblock.
     */
    Superblock* readSuper(Superblock* superblock, void* data) const override;

    /**
     * Creates an Superblock object for the actual file system type.
     * @param root the root dentry of the new superblock
     * @param s_dev the device number of the new superblock
     * @return a pointer to the Superblock object
     */
    Superblock* createSuper(uint32 s_dev) override;

    static DeviceFSType* getInstance();

protected:
    static DeviceFSType* instance_;
};
