#include "fs/devicefs/DeviceFSType.h"
#include "fs/devicefs/DeviceFSSuperblock.h"

DeviceFSType::DeviceFSType() :
    FileSystemType("devicefs")
{
}

DeviceFSType::~DeviceFSType()
{
}

Superblock *DeviceFSType::readSuper(Superblock *superblock, void*) const
{
  return superblock;
}

Superblock *DeviceFSType::createSuper(uint32 __attribute__((unused)) s_dev) const
{
  Superblock *super = DeviceFSSuperBlock::getInstance();
  return super;
}
