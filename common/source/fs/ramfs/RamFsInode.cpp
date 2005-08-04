
#include "fs/ramfs/RamFsInode.h"

#include "mm/kmalloc.h"
#include "util/string.h"
#include "assert.h"

#define BASIC_ALLOC 4096
#define ERROR_DNE "Error: the dentry does not exist."
#define ERROR_DU  "Error: inode is used."
#define ERROR_IC  "Error: invalid command (only for Directory)."
#define ERROR_NNE "Error: the name does not exist in the current directory."
#define ERROR_HLI "Error: hard link invalid."
#define ERROR_DNEILL "Error: the dentry does not exist in the link list."
#define ERROR_DES "Error: the dentry exists sub_directory"

//---------------------------------------------------------------------------
RamFsInode::RamFsInode(Superblock *super_block, uint32 inode_mode) :
    Inode(super_block, inode_mode)
{
  if(inode_mode == I_FILE)
    data_ = (int32*)kmalloc(BASIC_ALLOC);
  else
    data_ = 0;

  i_state_ = I_UNUSED;
  i_size_ = BASIC_ALLOC;
  i_count_ = 0;
  i_nlink_ = 0;
  i_dentry_ = 0;
}

//---------------------------------------------------------------------------
RamFsInode::~RamFsInode()
{
  assert(i_count_ != 0);
  assert(i_nlink_ != 0);
  assert(i_dentry_ == 0);
  assert(i_dentry_link_.is_empty() != true);
  
  if (data_)
  {
    kfree(data_);
  }
}

//---------------------------------------------------------------------------
int32 RamFsInode::readData(int32 offset, int32 size, int32 *buffer)
{
  if(i_mode_ == I_FILE)
  {
    int32 *ptr_offset = data_ + (offset / 4);

    if (memcpy(buffer, ptr_offset, size))
    {
      return 1;
    }
  }
  
  return 0;
}

//---------------------------------------------------------------------------
int32 RamFsInode::writeData(int32 offset, int32 size, int32 *buffer)
{
  if(i_mode_ == I_FILE)
  {
    int32 *ptr_offset = data_ + (offset / 4);
    
    if(memcpy(ptr_offset, buffer, size))
    {
      return 1;
    }
  }
  
  return 0;
}

//---------------------------------------------------------------------------
int32 RamFsInode::mknod(Dentry *dentry)
{
  if(dentry == 0)
  {
    // ERROR_DNE
    return -1;
  }
  
	if(i_count_ != 0)
 	{
 	  // ERROR_DU
 	  return -1;
 	}
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

//---------------------------------------------------------------------------
int32 RamFsInode::link(Dentry *dentry)
{
  if(dentry == 0)
  {
    // ERROR_DNE
    return -1;
  }

  if(i_mode_ == I_FILE)
  {
    i_nlink_++;
    i_count_++;
    i_dentry_link_.push_end(dentry);
    
    // dentry instantiate
    dentry->dentry_instantiate(this);
  }
  else
  {
    // ERROR_HLI
    return -1;
  }
  
  return 0;
}

//---------------------------------------------------------------------------
int32 RamFsInode::unlink(Dentry *dentry)
{
  if(dentry == 0)
  {
    // ERROR_DNE
    return -1;
  }

  if(i_mode_ == I_FILE)
  {
    if(i_dentry_link_.is_included(dentry) == false)
    {
      // ERROR_DNEILL
      return -1;
    }

    i_nlink_--;
    i_count_--;
    i_dentry_link_.remove(dentry);
    
    if(i_dentry_link_.is_empty() == false)
      return 0;
  }

  // dentry destantiate & remove correspondent dentry
  int32 allow_remove = dentry->dentry_destantiate();
  if(allow_remove == -1)
  {
    // ERROR_DES
    return -1;
  }
  else
  {
    Dentry *parent_dentry = dentry->get_parent();
    parent_dentry->d_child_remove(dentry);
    delete dentry;
  }
  
  return 0;
}

//---------------------------------------------------------------------------
int32 RamFsInode::rmdir(Dentry *sub_dentry)
{
  if(sub_dentry == 0)
  {
    // ERROR_DNE
    return -1;
  }
  
  if(i_mode_ == I_DIR)
  {
    if(i_dentry_->check_name(sub_dentry) == true)
    {
      // ???
      unlink(sub_dentry);
    }
    else
    {
      // ERROR_NNE
      return -1;
    }
  }
  else
  {
    // ERROR_IC
    return -1;
  }
  
  return 0;
}

//---------------------------------------------------------------------------
Dentry* RamFsInode::lookup(Dentry *dentry)
{
  if(dentry == 0)
  {
    // ERROR_DNE
    return 0;
  }
  
  if(i_mode_ == I_DIR)
  {
    if(i_dentry_->check_name(dentry) == true)
    {
      // ERROR_NNE
      return 0;
    }
    else
    {
      return dentry;
    }
  }
  else
  {
    // ERROR_IC
    return 0;
  }
  
  return dentry;
}

//---------------------------------------------------------------------------
