
#include "fs/ramfs/RamFileSystemType.h"
//#include "fs/ramfs/RamFsSuperblock.h"

//----------------------------------------------------------------------
RamFileSystemType::RamFileSystemType()
{
  fs_name_ = "ramfs";
  fs_flags_ = 0;
}

RamFileSystemType::~RamFileSystemType()
{
}

Superblock *RamFileSystemType::readSuper(int32 flags, const char* /*dev_name*/)
{
  //return (Superblock*)(new RamFsSuperblock);
}

