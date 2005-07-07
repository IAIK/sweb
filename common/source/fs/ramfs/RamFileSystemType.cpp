
#include "fs/ramfs/RamFileSystemType.h"

//----------------------------------------------------------------------
RamFileSystemType::RamFileSystemType()
{
  fs_name_ = "ramfs";
  fs_flags_ = 0;
}

RamFileSystemType::~RamFileSystemType()
{
}

Superblock *RamFileSystemType::readSuper(int32 flags, const char* dev_name)
{
}

