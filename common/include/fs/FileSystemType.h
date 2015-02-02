#ifndef FILE_SYSTEM_TYPE_H__
#define FILE_SYSTEM_TYPE_H__

#include "types.h"

class Superblock;
class Dentry;

/**
 * File system flag indicating if the system in question requires an device.
 */
#define FS_REQUIRES_DEV   0x0001 // located on a physical disk device
#define FS_NOMOUNT        0x0010 // Filesystem has no mount point

/**
 * The maximal number of file system types.
 */
#define MAX_FILE_SYSTEM_TYPES 16

/**
 * @class FileSystemType
 * FileSystemType is used to register the file system to the vfs.
 * It also reads the superblock from the block device.
 */
class FileSystemType
{

  protected:

    /**
     * the name of the File-system-type
     */
    const char *fs_name_;

    /**
     * the flags of the File-system-type
     */
    int32 fs_flags_;

  public:

    /**
     * contructor
     */
    FileSystemType(const char *fs_name);

    /**
     * destructor
     */
    virtual ~FileSystemType();

    /**
     * the assign operator
     */
    FileSystemType const &operator =(FileSystemType const &instance)
    {
      fs_name_ = instance.fs_name_;
      fs_flags_ = instance.fs_flags_;
      return (*this);
    }

    /**
     * get the name from the file-system-type
     * @return the file system name
     */
    const char* getFSName() const;

    /**
     * set the name to the file-system-type
     * @param fs_name the name to set
     */
    void setFSName(const char* fs_name);

    /**
     * get the flags from the file-system-type
     * @return the flags
     */
    int32 getFSFlags() const;

    /**
     * set the flags to the file-system-type
     * @param flags the flags to set
     */
    void setFSFlags(int32 fs_flags);

    /**
     * Reads the superblock from the device.
     * @param superblock is the superblock to fill with data.
     * @param data is the data given to the mount system call.
     * @return is a pointer to the resulting superblock.
     */
    virtual Superblock *readSuper(Superblock *superblock, void *data) const;

    /**
     * Creates an Superblock object for the actual file system type.
     * @param s_dev a valid device number or -1 if no block device is available
     *              (e.g. for pseudo file systems)
     * @return a pointer to the Superblock object, 0 if wasn't possible to
     * create a Superblock with the given device number
     */
    virtual Superblock *createSuper(Dentry *root, uint32 s_dev) const;

};

#endif // FileSystemType_h___
