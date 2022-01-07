#include "fs/devicefs/DeviceFSType.h"
#include "fs/devicefs/DeviceFSSuperblock.h"

DeviceFSType::DeviceFSType() :
    FileSystemType("devicefs")
{
}

Superblock *DeviceFSType::readSuper(Superblock *superblock, [[maybe_unused]] void* data) const
{
  return superblock;
}

Superblock *DeviceFSType::createSuper(Dentry __attribute__((unused)) *root, uint32 __attribute__((unused)) s_dev) const
{
  Superblock *super = DeviceFSSuperBlock::getInstance();
  return super;
}
