// Projectname: SWEB
// Simple operating system for educational purposes

#include "fs/minixfs/MinixFSType.h"
#include "fs/minixfs/MinixFSSuperblock.h"

//----------------------------------------------------------------------
MinixFSType::MinixFSType()
{
  fs_name_ = "minixfs";
  fs_flags_ = 0;
}

//----------------------------------------------------------------------
MinixFSType::~MinixFSType()
{
}

//----------------------------------------------------------------------
Superblock *MinixFSType::readSuper(Superblock *superblock, void*) const
{
  return superblock;
}

//----------------------------------------------------------------------
Superblock *MinixFSType::createSuper(Dentry *root) const
{
  Superblock *super = new MinixFSSuperblock(root);
  //super->setRoot( root );
  return super;
}
