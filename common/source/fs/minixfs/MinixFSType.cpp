#include "MinixFSType.h"

#include "BDManager.h"
#include "BDVirtualDevice.h"
#include "MinixFSSuperblock.h"

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

  auto bdev = BDManager::instance().getDeviceByNumber(s_dev);
  bdev->setBlockSize(BLOCK_SIZE);
  uint64 size = bdev->getNumBlocks()*bdev->getBlockSize();

  Superblock *super = new MinixFSSuperblock(this, s_dev, 0, size);
  return super;
}
