// FileSystemInfo.cpp

#include "fs/FileSystemInfo.h"
#include "fs/Dentry.h"
#include "fs/VfsMount.h"


/// A global object for information about the current position in the FileSystem.
/// TODO this has to be integrated in the ussr spave process code.
/// Every process needs one of these.
FileSystemInfo fs_info;

//----------------------------------------------------------------------
FileSystemInfo::FileSystemInfo()
{
}

