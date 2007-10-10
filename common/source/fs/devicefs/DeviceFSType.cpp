/**
 * @file DeviceFSType.cpp
 */

#include "fs/devicefs/DeviceFSType.h"
#include "fs/devicefs/DeviceFSSuperblock.h"

DeviceFSType::DeviceFSType()
{
  fs_name_ = "devicefs";
  fs_flags_ = 0;
}


DeviceFSType::~DeviceFSType()
{}


Superblock *DeviceFSType::readSuper ( Superblock *superblock, void* ) const
{
  return superblock;
}


Superblock *DeviceFSType::createSuper ( Dentry *root, uint32 s_dev ) const
{
  UNUSED_ARG(root);
  UNUSED_ARG(s_dev);
  Superblock *super = DeviceFSSuperBlock::getInstance();
  return super;
}
