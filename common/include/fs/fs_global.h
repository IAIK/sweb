/// @file global objects and variables needed by the Virtual file System

#include "fs/VirtualFileSystem.h"
#include "fs/FileSystemInfo.h"

/// Global VirtualFileSystem object
extern VirtualFileSystem vfs;

/// A global object for information about the current position in the FileSystem.
/// TODO this has to be integrated in the ussr spave process code.
/// Every process needs one of these.
extern FileSystemInfo fs_info;
