/**
 * @file RamFSType.h
 */

#ifndef RAM_FS_TYPE_H__
#define RAM_FS_TYPE_H__

#include "fs/FileSystemType.h"

/**
 * @class RamFSType
 */
class RamFSType : public FileSystemType
{
  public:

    /**
     * constructor
     */
    RamFSType();

    /**
     * destructor
     */
    virtual ~RamFSType();

    /**
     * Reads the superblock from the device.
     * @param superblock is the superblock to fill with data.
     * @param data is the data given to the mount system call.
     * @return is a pointer to the resulting superblock.
     */
    virtual Superblock *readSuper(Superblock *superblock, void *data) const;

    /**
     * Creates an Superblock object for the actual file system type.
     * @return a pointer to the Superblock object
     */
    virtual Superblock *createSuper(Dentry *root, uint32 s_dev) const;
};

#endif
