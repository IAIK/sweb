/**
 * @file Superblock.cpp
 */

#include "fs/Superblock.h"
#include "assert.h"
#include "Dentry.h"
#include "Inode.h"
#include "File.h"

Superblock::~Superblock()
{}


void Superblock::delete_inode ( Inode *inode )
{
  assert ( inode != 0 );
  int32 del_inode = dirty_inodes_.remove ( inode );
  if ( del_inode == -1 )
    del_inode = used_inodes_.remove ( inode );
  assert ( del_inode != -1 );
  delete inode;
}


Dentry *Superblock::getRoot()
{
  return s_root_;
}


Dentry *Superblock::getMountPoint()
{
  return mounted_over_;
}

/*
nt32 Superblock::insertOpenedFiles(File* file)
{
  if(file == 0)
  {
    // ERROR_FNE
    return -1;
  }

  if(s_files_.included(file) == true)
  {
    // ERROR_FE
    return -1;
  }
  s_files_.pushBack(file);

  return 0;
}

int32 Superblock::removeOpenedFiles(File* file)
{
  if(file == 0)
  {
    // ERROR_FNE
    return -1;
  }

  if(s_files_.included(file) == true)
    s_files_.remove(file);
  else
  {
    // ERROR_FNE
    return -1;
  }

  return 0;
}
*/


