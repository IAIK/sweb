
//
// CVS Log Info for $RCSfile: PseudoFsSuperblock.cpp,v $
//
// $Id: PseudoFsSuperblock.cpp,v 1.4 2005/09/28 20:11:36 qiangchen Exp $
// $Log: PseudoFsSuperblock.cpp,v $
// Revision 1.3  2005/09/20 15:32:30  davrieb
// fix loop reading creating the inodes
//
// Revision 1.2  2005/09/17 09:33:55  davrieb
// finished pseudofs code
//
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

#include "console/kprintf.h"

//----------------------------------------------------------------------
PseudoFsSuperblock::PseudoFsSuperblock(Dentry* s_root, uint32 s_dev) :
  RamFsSuperblock(s_root, s_dev)
{
  PseudoFS *pfs = PseudoFS::getInstance();

  uint32 num_files = pfs->getNumFiles();
  for (uint32 fnum = 0; fnum < num_files; ++fnum)
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
Inode* PseudoFsSuperblock::createInode(Dentry* dentry, uint32 type)
{
  Inode *inode = (Inode*)(new PseudoFsInode(this, type));
  int32 inode_init = inode->mknod(dentry);

  all_inodes_.pushBack(inode);
  return inode;
}

