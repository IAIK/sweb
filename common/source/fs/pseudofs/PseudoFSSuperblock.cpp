/**
 * @file PseudoFSSuperblock.cpp
 */

#include "fs/PseudoFS.h"

#include "fs/pseudofs/PseudoFSSuperblock.h"
#include "fs/pseudofs/PseudoFSInode.h"
#include "fs/Dentry.h"
#include "fs/Inode.h"


PseudoFSSuperblock::PseudoFSSuperblock ( Dentry* s_root, uint32 s_dev ) :
    RamFSSuperblock ( s_root, s_dev )
{
  PseudoFS *pfs = PseudoFS::getInstance();

  uint32 num_files = pfs->getNumFiles();
  for ( uint32 fnum = 0; fnum < num_files; ++fnum )
  {
    char *file_name = pfs->getFileNameByNumber ( fnum );
    Dentry* fdntr = new Dentry ( s_root_ );
    fdntr->setName ( file_name );
    createInode ( fdntr, I_FILE );
  }
}


PseudoFSSuperblock::~PseudoFSSuperblock()
{}


Inode* PseudoFSSuperblock::createInode ( Dentry* dentry, uint32 type )
{
  Inode *inode = ( Inode* ) ( new PseudoFSInode ( this, type ) );
  inode->mknod ( dentry );

  all_inodes_.pushBack ( inode );
  return inode;
}

