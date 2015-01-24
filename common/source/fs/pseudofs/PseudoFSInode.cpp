/**
 * @file PseudoFSInode.cpp
 */

#include "fs/pseudofs/PseudoFSInode.h"

#include "fs/PseudoFS.h"
#include "fs/Dentry.h"

#include "util/string.h"


PseudoFSInode::PseudoFSInode ( Superblock *super_block, uint32 inode_type )
    : RamFSInode ( super_block, inode_type )
{}


PseudoFSInode::~PseudoFSInode()
{}


int32 PseudoFSInode::readData ( uint32 offset, uint32 size, char *buffer )
{
  if ( i_type_ == I_FILE )
  {
    char* my_name = i_dentry_->getName();

    PseudoFS* pfs = PseudoFS::getInstance();
    uint8* filp =  pfs->getFilePtr ( my_name );
    PseudoFS::FileIndexStruct* findx = pfs->getFileIndex ( my_name );

    uint32 length = findx->file_length;
    if ( length < ( offset + size ) )
    {
      return -1;
    }

    filp += offset;
    memcpy ( buffer, filp, size );
    return 0;
  }

  return -1;
}


int32 PseudoFSInode::writeData ( uint32 /*offset*/, uint32 /*size*/, char * /*buffer*/ )
{
  return -1;
}

