
//
// CVS Log Info for $RCSfile: PseudoFsInode.cpp,v $
//
// $Id: PseudoFsInode.cpp,v 1.2 2005/09/28 20:11:36 qiangchen Exp $
// $Log: PseudoFsInode.cpp,v $
// Revision 1.1  2005/09/17 09:33:55  davrieb
// finished pseudofs code
//
//
//

#include "fs/pseudofs/PseudoFsInode.h"

#include "fs/PseudoFS.h"
#include "fs/Dentry.h"

#include "util/string.h"


//----------------------------------------------------------------------
PseudoFsInode::PseudoFsInode(Superblock *super_block, uint32 inode_type)
  : RamFsInode(super_block, inode_type)
{
}

//----------------------------------------------------------------------
PseudoFsInode::~PseudoFsInode()
{
}

//----------------------------------------------------------------------
int32 PseudoFsInode::readData(uint32 offset, uint32 size, char *buffer)
{
  if (i_type_ == I_FILE)
  {
    char* my_name = i_dentry_->getName();

    PseudoFS* pfs = PseudoFS::getInstance();
    uint8* filp =  pfs->getFilePtr(my_name);
    PseudoFS::FileIndexStruct* findx = pfs->getFileIndex(my_name);

    uint32 length = findx->file_length;
    if (length < (offset + size))
    {
      return -1;
    }

    filp += offset;
    memcpy(buffer, filp, size);
    return 0;
  }

  return -1;
}

//----------------------------------------------------------------------
int32 PseudoFsInode::writeData(uint32 /*offset*/, uint32 /*size*/, char * /*buffer*/)
{
  return -1;
}

