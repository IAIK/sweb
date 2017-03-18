#pragma once

#include "fs/FileSystemType.h"

class DeviceFSType : public FileSystemType
{
  public:
    DeviceFSType();

    virtual ~DeviceFSType();

    /**
     * Reads the superblock from the device.
     * @param superblock is the superblock to fill with data.
     * @param data is the data given to the mount system call.
     * @return is a pointer to the resulting superblock.
     */
    virtual Superblock *readSuper(Superblock *superblock, void *data) const;

    /**
     * Creates an Superblock object for the actual file system type.
     * @param root the root dentry of the new superblock
     * @param s_dev the device number of the new superblock
     * @return a pointer to the Superblock object
     */
    virtual Superblock *createSuper(Dentry *root, uint32 s_dev) const;
};

