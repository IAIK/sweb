// FileSystemInfo.cpp

#include "fs/FileSystemInfo.h"
#include "fs/Dentry.h"
#include "fs/VfsMount.h"


/// A global object for information about the current position in the FileSystem.
/// TODO this has to be integrated in the ussr spave process code.
/// Every process needs one of these.
FileSystemInfo fs_info;


FileSystemInfo::FileSystemInfo()
{
  root_ = 0;
  root_mnt_ = 0;
  current_position_ = 0;
  current_position_mnt_ = 0;
  alt_root_ = 0;
  alt_root_mnt_ = 0;
}

