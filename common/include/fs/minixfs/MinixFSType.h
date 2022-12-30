#pragma once

#include "FileSystemType.h"

class MinixFSType : public FileSystemType
{
public:
    MinixFSType();
    ~MinixFSType() override = default;

    /**
     *  reads the superblock from the device
     * @param superblock a pointer to the resulting superblock
     * @param data the data given to the mount system call
     * @return the superblock
     */
    Superblock* readSuper(Superblock* superblock, void* data) const override;

    /**
     * creates an Superblock object for the actual file system type
     * @param root the root dentry
     * @param s_dev the device number
     */
    Superblock* createSuper(uint32 s_dev) override;
};
