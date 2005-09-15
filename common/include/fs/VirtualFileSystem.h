// Projectname: SWEB
// Simple operating system for educational purposes

#ifndef VirtualFileSystem_h___
#define VirtualFileSystem_h___

#include "types.h"
#include "fs/PointList.h"

/// File system flag indicating if the system in question requires an device.
#define FS_REQUIRES_DEV   0x0001 // located on a physical disk device
#define FS_NOMOUNT        0x0010 // Filesystem has no mount point

/// The maximal number of file system types.
#define MAX_FILE_SYSTEM_TYPES 16

class Superblock;
class FileSystemType;
class VfsMount;
class FileSystemInfo;

class VirtualFileSystem
{
  protected:

    PointList<Superblock> superblocks_;

    /// PointList of mounted Filesystems
    PointList<VfsMount> mounts_;

    /// A null-terminated array of file system types.
    PointList<FileSystemType> file_system_types_;

  public:

    /// The constructor
    VirtualFileSystem();

    /// The destructor
    ~VirtualFileSystem();

    /// register the file-system-typt to the vfs
    int32 registerFileSystem(FileSystemType *file_system_type);

    /// unregister the file-system-typt to the vfs
    int32 unregisterFileSystem(FileSystemType *file_system_type);

    /// The getFsType function receives a filesystem name as its parameter, scans
    /// the list of registered filesystems looking at the fs_name field of their
    /// descriptors, and returns a pointer to the corresponding FileSystemType
    /// object, if is present.
    FileSystemType *getFsType(const char* fs_name);

    int32 mount(char* path, char* fs_name, int32 mode);

    int32 root_mount(char* fs_name, int32 mode);

    int32 rootUmount();

    /// Get the FileSystemInfo object of the current Process
    FileSystemInfo *getFSInfo();
};

#endif
