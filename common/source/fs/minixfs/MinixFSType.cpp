#include "MinixFSType.h"
#include "MinixFSSuperblock.h"
#include "BDManager.h"
#include "BDVirtualDevice.h"

MinixFSType::MinixFSType() : FileSystemType("minixfs")
{
    fs_flags_ |= FS_REQUIRES_DEV;
}


Superblock *MinixFSType::readSuper(Superblock *superblock, [[maybe_unused]]void* data) const
{
  return superblock;
}


Superblock *MinixFSType::createSuper(uint32 s_dev)
{
  if (s_dev == (uint32) -1)
    return nullptr;

  BDManager::getInstance()->getDeviceByNumber(s_dev)->setBlockSize(BLOCK_SIZE);
  Superblock *super = new MinixFSSuperblock(this, s_dev, 0);
  return super;
}
