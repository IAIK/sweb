// Projectname: SWEB
// Simple operating system for educational purposes

#ifndef _MINIXFS_TYPE_H_
#define _MINIXFS_TYPE_H_

#include "fs/FileSystemType.h"

class MinixFSType : public FileSystemType
{
  public:

    /// constructor
    MinixFSType();

    /// destructor
    virtual ~MinixFSType();

    /// Reads the superblock from the device.
    ///
    /// @return is a pointer to the resulting superblock.
    /// @param superblock is the superblock to fill with data.
    /// @param data is the data given to the mount system call.
    virtual Superblock *readSuper(Superblock *superblock, void *data) const;

    /// Creates an Superblock object for the actual file system type.
    ///
    /// @return a pointer to the Superblock object
    virtual Superblock *createSuper(Dentry *root, uint32 s_dev) const;
};

#endif
