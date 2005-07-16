
#include "fs/ramfs/RamFsInode.h"

#include "mm/kmalloc.h"
#include "util/string.h"
#include "assert.h"

#define BASIC_ALLOC 4096

//---------------------------------------------------------------------------
RamFsInode::RamFsInode(Superblock *super_block, uint32 inode_mode) :
    Inode(super_block, inode_mode)
{
  data_ = (int32*)kmalloc(BASIC_ALLOC);
  i_state_ = I_UNUSED;
  i_size_ = BASIC_ALLOC;
  i_count_ = 0;
  i_dentry_ = 0;
}

//---------------------------------------------------------------------------
RamFsInode::~RamFsInode()
{
  assert(i_count_ != 0);
  assert(i_dentry_ == 0);
  
  if (data_)
  {
    kfree(data_);
  }
}

//---------------------------------------------------------------------------
int32 RamFsInode::readData(int32 offset, int32 size, int32 *buffer)
{
  int32 *ptr_offset = data_ + (offset / 4);

  if (memcpy(buffer, ptr_offset, size))
  {
    return 1;
  }

  return 0;
}

//---------------------------------------------------------------------------
int32 RamFsInode::mknod(Dentry *dentry)
{
  if((dentry == 0) || (i_mode_ != I_DIR))
    return -1;
  
  i_count_++;
  i_dentry_ = dentry;
  dentry->set_inode(this);
  dentry->increment_dcount();
  return 0;
}

//---------------------------------------------------------------------------
int32 RamFsInode::mkdir(Dentry *dentry)
{
  return(mkdir(dentry));
}

//---------------------------------------------------------------------------
int32 RamFsInode::create(Dentry *dentry)
{
  return(mkdir(dentry));
}