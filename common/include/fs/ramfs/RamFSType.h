#pragma once

#include "fs/FileSystemType.h"

class RamFSType : public FileSystemType
{
  public:
    RamFSType();
    ~RamFSType() override = default;

    /**
     * Reads the superblock from the device.
     * @param superblock is the superblock to fill with data.
     * @param data is the data given to the mount system call.
     * @return is a pointer to the resulting superblock.
     */
    Superblock *readSuper(Superblock *superblock, void *data) const override;

    /**
     * Creates an Superblock object for the actual file system type.
     * @return a pointer to the Superblock object
     */
    Superblock *createSuper(uint32 s_dev) override;
};
