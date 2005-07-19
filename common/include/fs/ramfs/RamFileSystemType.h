
#ifndef RAMFILESYSTEMTYPE_H__
#define RAMFILESYSTEMTYPE_H__


#include "fs/FileSystemType.h"


class RamFileSystemType : public FileSystemType
{

  public:

    RamFileSystemType();

    virtual ~RamFileSystemType();

    /// Reads the superblock from the device.
    ///
    /// @return is a pointer to the resulting superblock.
    /// @param superblock is the superblock to fill with data.
    /// @param data is the data given to the mount system call.
    Superblock *readSuper(Superblock *superblock, void *dataa);

};

#endif

