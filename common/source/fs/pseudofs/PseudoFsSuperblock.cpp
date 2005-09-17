
//
// CVS Log Info for $RCSfile: PseudoFsSuperblock.cpp,v $
//
// $Id: PseudoFsSuperblock.cpp,v 1.2 2005/09/17 09:33:55 davrieb Exp $
// $Log: PseudoFsSuperblock.cpp,v $
// Revision 1.1  2005/09/16 16:27:40  davrieb
// add pseudofs
//
//
//

#include "fs/PseudoFS.h"

#include "fs/pseudofs/PseudoFsSuperblock.h"
#include "fs/pseudofs/PseudoFsInode.h"
#include "fs/Dentry.h"
#include "fs/Inode.h"


//----------------------------------------------------------------------
PseudoFsSuperblock::PseudoFsSuperblock(Dentry* s_root) :
  RamFsSuperblock(s_root)
{
  PseudoFS *pfs = PseudoFS::getInstance();

  uint32 num_files = pfs->getNumFiles();
  for (uint32 fnum = 0; fnum < num_files; ++num_files)
  {
    char *file_name = pfs->getFileNameByNumber(fnum);
    Dentry* fdntr = new Dentry(s_root_);
    fdntr->setName(file_name);
    createInode(fdntr, I_FILE);
  }
}

//----------------------------------------------------------------------
PseudoFsSuperblock::~PseudoFsSuperblock()
{
}

//----------------------------------------------------------------------
Inode* PseudoFsSuperblock::createInode(Dentry* dentry, uint32 mode)
{
  Inode *inode = (Inode*)(new PseudoFsInode(this, mode));
  int32 inode_init = inode->mknod(dentry);
  assert(inode_init == 0);

  all_inodes_.pushBack(inode);
  return inode;
}

