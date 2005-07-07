
#ifndef RAMFILESYSTEMTYPE_H__
#define RAMFILESYSTEMTYPE_H__

#include "fs/VirtualFileSystem.h"


class RamFileSystemType : public FileSystemType
{

  protected:

    char* fs_name_;

    int32 fs_flags_;

  public:

    RamFileSystemType();

    virtual ~RamFileSystemType();

    /// Reads the superblock from the device.
    ///
    /// @return is a pointer to the resulting superblock.
    /// @param flags contains the mount flags.
    /// @param dev_name is the name of the device where the superblock will be read from.
    Superblock *readSuper(int32 flags, const char* dev_name);

};

#endif

