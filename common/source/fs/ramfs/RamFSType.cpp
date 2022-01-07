#include "fs/ramfs/RamFSType.h"
#include "fs/ramfs/RamFSSuperblock.h"


RamFSType::RamFSType() : FileSystemType("ramfs")
{
}


Superblock *RamFSType::readSuper ( Superblock *superblock, [[maybe_unused]]void* data) const
{
  return superblock;
}


Superblock *RamFSType::createSuper ( Dentry *root, uint32 s_dev ) const
{
  Superblock *super = new RamFSSuperblock ( root, s_dev );
  return super;
}
