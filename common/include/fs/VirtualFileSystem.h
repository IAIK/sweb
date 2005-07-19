
//
// CVS Log Info for $RCSfile: VirtualFileSystem.h,v $
//
// $Id: VirtualFileSystem.h,v 1.6 2005/07/19 17:11:03 davrieb Exp $
// $Log: VirtualFileSystem.h,v $
// Revision 1.5  2005/07/16 13:22:00  davrieb
// rrename List in fs to PointList to avoid name clashes
//
// Revision 1.4  2005/07/07 13:36:58  davrieb
// fix include of kmalloc
//
// Revision 1.3  2005/07/07 12:31:19  davrieb
// add ramfs and all changes it caused
//
// Revision 1.2  2005/06/01 09:20:36  davrieb
// add all changes to fs
//
// Revision 1.1  2005/05/10 16:42:32  davrieb
// add first attempt to write a virtual file system
//
//

#ifndef VirtualFileSystem_h__
#define VirtualFileSystem_h__

#include "types.h"
#include "mm/kmalloc.h"
// #include "Superblock.h"

/// File system flag indicating if the system in question requires an device.
#define FS_REQUIRES_DEV   0x0001 // located on a physical disk device
#define FS_NOMOUNT        0x0010 // Filesystem has no mount point

/// The maximal number of file system types.
#define MAX_FILE_SYSTEM_TYPES 16

#include "kernel/List.h"

class Superblock;
class FileSystemType;

class VirtualFileSystem
{
  protected:

    List<Superblock*> superblocks_;

    /// A null-terminated array of file system types.
    List<FileSystemType*> file_system_types_;

  public:

    /// The constructor
    VirtualFileSystem();

    /// The destructor
    ~VirtualFileSystem();

    int32 registerFileSystem(FileSystemType *file_system_type);

    int32 unregisterFileSystem(FileSystemType *file_system_type);

    /// The getFsType function receives a filesystem name as its parameter, scans
    /// the list of registered filesystems looking at the fs_name field of their
    /// descriptors, and returns a pointer to the corresponding FileSystemType
    /// object, if is present.
    FileSystemType *getFsType(const char* fs_name);

};

#endif
