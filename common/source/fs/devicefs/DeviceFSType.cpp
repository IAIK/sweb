// Projectname: SWEB
// Simple operating system for educational purposes

#include "fs/devicefs/DeviceFSType.h"
#include "fs/devicefs/DeviceFSSuperblock.h"

//----------------------------------------------------------------------
DeviceFSType::DeviceFSType()
{
  fs_name_ = "devicefs";
  fs_flags_ = 0;
}

//----------------------------------------------------------------------
DeviceFSType::~DeviceFSType()
{
}

//----------------------------------------------------------------------
Superblock *DeviceFSType::readSuper(Superblock *superblock, void*) const
{
  return superblock;
}

//----------------------------------------------------------------------
Superblock *DeviceFSType::createSuper(Dentry *root) const
{
  Superblock *super = DeviceFSSuperBlock::getInstance();
  //super->setRoot( root );
  return super;
}
