
//
// CVS Log Info for $RCSfile: Superblock.cpp,v $
//
// $Id: Superblock.cpp,v 1.5 2005/08/11 16:34:28 qiangchen Exp $
// $Log: Superblock.cpp,v $
// Revision 1.4  2005/07/21 18:07:04  davrieb
// mount of the root directory
//
// Revision 1.3  2005/07/07 12:31:48  davrieb
// add ramfs
//
// Revision 1.2  2005/06/01 09:20:36  davrieb
// add all changes to fs
//
// Revision 1.1  2005/05/10 16:42:29  davrieb
// add first attempt to write a virtual file system
//
//

#include "fs/Superblock.h"
#include "assert.h"

//------------------------------------------------------------------
Superblock::~Superblock()
{
  assert(s_inode_dirty_.is_empty() != false);
  assert(s_inode_used_.is_empty() != false);
  assert(s_files_.is_empty() != false);
  s_type_ = 0;

  delete s_root_;
}
//------------------------------------------------------------------
void Superblock::delete_inode(Inode *inode)
{
  assert(inode != 0);
  int32 del_inode = s_inode_dirty_.remove(inode);
  if(del_inode == -1)
    del_inode = s_inode_used_.remove(inode);
  assert(del_inode != -1);
  delete inode;
}

//----------------------------------------------------------------------
int32 Superblock::insert_opened_files(File* file)
{
  if(file == 0)
  {
    // ERROR_FNE
    return -1;
  }
  
  if(s_files_.is_included(file) == true)
  {
    // ERROR_FE
    return -1;
  }
  s_files_.push_end(file);
  
  return 0;
}

//----------------------------------------------------------------------
int32 Superblock::remove_opened_files(File* file)
{
  if(file == 0)
  {
    // ERROR_FNE
    return -1;
  }
  
  if(s_files_.is_included(file) == true)
    s_files_.remove(file);
  else
  {
    // ERROR_FNE
    return -1;
  }
  
  return 0;
}
