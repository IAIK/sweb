// FileSystemInfo.cpp

#include "FileSystemInfo.h"
#include "fs/Dentry.h"
#include "fs/VfsMount.h"

FileSystemInfo::FileSystemInfo()
{
  root_ = 0;
  root_mnt_ = 0;
  current_position_ = 0;
  current_position_mnt_ = 0;
  alt_root_ = 0;
  alt_root_mnt_ = 0;
}