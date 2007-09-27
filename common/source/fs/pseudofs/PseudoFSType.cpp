/**
 * @file PseudoFSType.cpp
 */

#include "fs/pseudofs/PseudoFSType.h"
#include "fs/pseudofs/PseudoFSSuperblock.h"

PseudoFSType::PseudoFSType()
{
  fs_name_ = "pseudofs";
  fs_flags_ = 0;
}


PseudoFSType::~PseudoFSType()
{}


Superblock *PseudoFSType::readSuper ( Superblock *superblock, void* ) const
{
  return superblock;
}


Superblock *PseudoFSType::createSuper ( Dentry *root, uint32 s_dev ) const
{
  Superblock *super = new PseudoFSSuperblock ( root, s_dev );
  return super;
}

