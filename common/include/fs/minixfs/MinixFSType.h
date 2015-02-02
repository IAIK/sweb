#ifndef _MINIXFS_TYPE_H_
#define _MINIXFS_TYPE_H_

#include "FileSystemType.h"

/**
 * @class MinixFSType is used to register the file system to the vfs.
 * It also reads the minix superblock from the block device.
 */
class MinixFSType : public FileSystemType
{
  public:

    /**
     * constructor
     */
    MinixFSType();

    /**
     * destructor
     */
    virtual ~MinixFSType();

    /**
     *  reads the superblock from the device
     * @param superblock a pointer to the resulting superblock
     * @param data the data given to the mount system call
     * @return the superblock
     */
    virtual Superblock *readSuper(Superblock *superblock, void *data) const;

    /**
     * creates an Superblock object for the actual file system type
     * @param root the root dentry
     * @param s_dev the device number
     */
    virtual Superblock *createSuper(Dentry *root, uint32 s_dev) const;
};

#endif
