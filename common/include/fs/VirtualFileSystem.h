
//
// CVS Log Info for $RCSfile: VirtualFileSystem.h,v $
//
// $Id: VirtualFileSystem.h,v 1.12 2005/09/14 14:22:16 davrieb Exp $
// $Log: VirtualFileSystem.h,v $
// Revision 1.11  2005/09/12 17:55:53  qiangchen
// test the VFS (vfsvfs__syscall)
//
// Revision 1.10  2005/09/10 19:25:27  qiangchen
//  21:24:09 up 14:16,  3 users,  load average: 0.08, 0.09, 0.14
// USER     TTY      FROM              LOGIN@   IDLE   JCPU   PCPU WHAT
// chen     :0       -                12:11   ?xdm?   1:01m  1.35s /usr/bin/gnome-
// chen     pts/0    :0.0             12:15    1.00s  0.34s  0.03s cvs commit
// chen     pts/1    :0.0             12:33    5:23m  3.13s  0.04s -bash
//
// Revision 1.9  2005/09/02 17:57:58  davrieb
// preparations to  build a standalone filesystem testsuite
//
// Revision 1.8  2005/08/11 16:46:57  davrieb
// add PathWalker
//
// Revision 1.7  2005/07/21 18:07:03  davrieb
// mount of the root directory
//
// Revision 1.6  2005/07/19 17:11:03  davrieb
// put filesystemtype into it's own file
//
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
#include "fs/PointList.h"

/// File system flag indicating if the system in question requires an device.
#define FS_REQUIRES_DEV   0x0001 // located on a physical disk device
#define FS_NOMOUNT        0x0010 // Filesystem has no mount point

/// The maximal number of file system types.
#define MAX_FILE_SYSTEM_TYPES 16

// #include "kernel/List.h"

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

    int32 registerFileSystem(FileSystemType *file_system_type);

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

