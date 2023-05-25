#include "MinixFSFile.h"

#include "File.h"
#include "Inode.h"
#include "MinixFSInode.h"

MinixFSFile::MinixFSFile(Inode* inode, Dentry* dentry, uint32 flag) :
    SimpleFile(inode, dentry, flag)
{
}

int32 MinixFSFile::flush()
{
  ((MinixFSInode *) f_inode_)->flush();
  return 0;
}
