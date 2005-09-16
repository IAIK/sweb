
//
// CVS Log Info for $RCSfile: PseudoFsSuperblock.cpp,v $
//
// $Id: PseudoFsSuperblock.cpp,v 1.1 2005/09/16 16:27:40 davrieb Exp $
// $Log$
//
//

#include "fs/PseudoFS.h"

#include "fs/pseudofs/PseudoFsSuperblock.h"
#include "fs/Dentry.h"


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

