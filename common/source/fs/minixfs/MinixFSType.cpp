#include "MinixFSType.h"
#include "MinixFSSuperblock.h"
#include "BDManager.h"
#include "BDVirtualDevice.h"

MinixFSType::MinixFSType() : FileSystemType("minixfs")
{
}


MinixFSType::~MinixFSType()
{
}


Superblock *MinixFSType::readSuper(Superblock *superblock, void*) const
{
  return superblock;
}


Superblock *MinixFSType::createSuper(Dentry *root, uint32 s_dev) const
{
  if (s_dev == (uint32) -1)
    return 0;

  BDManager::getInstance()->getDeviceByNumber(s_dev)->setBlockSize(BLOCK_SIZE);
  Superblock *super = new MinixFSSuperblock(root, s_dev, 0);
  return super;
}
