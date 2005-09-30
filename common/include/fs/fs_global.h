/// @file global objects and variables needed by the Virtual file System

#include "fs/VirtualFileSystem.h"
#include "fs/FileSystemInfo.h"
#include "fs/PathWalker.h"
#include "fs/VfsSyscall.h"
#include "fs/FileDescriptor.h"

/// Global VirtualFileSystem object
extern VirtualFileSystem vfs;

/// A global object for information about the current position in the FileSystem.
/// TODO this has to be integrated in the ussr spave process code.
/// Every process needs one of these.
extern FileSystemInfo fs_info;

/// the pathWalker object
/// follow the inode of the corresponding file pathname
extern PathWalker path_walker;

/// the syscall of the virtual filesystem
extern VfsSyscall vfs_syscall;

/// the global file descriptor list
extern PointList<FileDescriptor> global_fd;
