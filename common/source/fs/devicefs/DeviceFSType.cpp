#include "fs/devicefs/DeviceFSType.h"
#include "fs/devicefs/DeviceFSSuperblock.h"

DeviceFSType* DeviceFSType::instance_ = nullptr;

DeviceFSType::DeviceFSType() :
    RamFSType()
{
    fs_name_ = "devicefs";
}

Superblock* DeviceFSType::readSuper(Superblock *superblock, void*) const
{
  return superblock;
}

Superblock* DeviceFSType::createSuper([[maybe_unused]] uint32  s_dev)
{
  Superblock *super = DeviceFSSuperBlock::getInstance();
  return super;
}

DeviceFSType* DeviceFSType::getInstance()
{
    if (!instance_)
        instance_ = new DeviceFSType();
    return instance_;
}
