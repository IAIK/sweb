
#include "fs/pseudofs/PseudoFileSystemType.h"
#include "fs/pseudofs/PseudoFsSuperblock.h"

//----------------------------------------------------------------------
PseudoFileSystemType::PseudoFileSystemType()
{
  fs_name_ = "pseudofs";
  fs_flags_ = 0;
}

//----------------------------------------------------------------------
PseudoFileSystemType::~PseudoFileSystemType()
{
}

//----------------------------------------------------------------------
Superblock *PseudoFileSystemType::readSuper(Superblock *superblock, void*) const
{
  return superblock;
}

//----------------------------------------------------------------------
Superblock *PseudoFileSystemType::createSuper(Dentry *root) const
{
  Superblock *super = new PseudoFsSuperblock(root);
  return super;
}

