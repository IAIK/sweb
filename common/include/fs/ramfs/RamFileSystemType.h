// Projectname: SWEB
// Simple operating system for educational purposes

#ifndef RAMFILESYSTEMTYPE_H___
#define RAMFILESYSTEMTYPE_H___

#include "fs/FileSystemType.h"

class RamFileSystemType : public FileSystemType
{
  public:

    /// constructor
    RamFileSystemType();

    /// destructor
    virtual ~RamFileSystemType();

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
