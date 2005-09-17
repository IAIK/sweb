
#include "fs/ramfs/RamFsInode.h"

#include "mm/kmalloc.h"
#include "util/string.h"
#include "assert.h"
#include "fs/ramfs/RamFsSuperblock.h"
#include "fs/Dentry.h"

#include "console/kprintf.h"

#define BASIC_ALLOC 256
#define ERROR_DNE "Error: the dentry does not exist.\n"
#define ERROR_DU  "Error: inode is used.\n"
#define ERROR_IC  "Error: invalid command (only for Directory).\n"
#define ERROR_NNE "Error: the name does not exist in the current directory.\n"
#define ERROR_HLI "Error: hard link invalid.\n"
#define ERROR_DNEILL "Error: the dentry does not exist in the link list.\n"
#define ERROR_DEC "Error: the dentry exists child.\n"

//---------------------------------------------------------------------------
RamFsInode::RamFsInode(Superblock *super_block, uint32 inode_mode) :
    Inode(super_block, inode_mode)
{
  if(inode_mode == I_FILE)
    data_ = (char*)kmalloc(BASIC_ALLOC);
  else
    data_ = 0;

  i_size_ = BASIC_ALLOC;
  i_nlink_ = 0;
  i_dentry_ = 0;
}

//---------------------------------------------------------------------------
RamFsInode::~RamFsInode()
{
  if (data_)
  {
    kfree(data_);
  }
}

//---------------------------------------------------------------------------
int32 RamFsInode::readData(int32 offset, int32 size, char *buffer)
{
  if(i_mode_ == I_FILE)
  {
    char *ptr_offset = data_ + offset;
    memcpy(buffer, ptr_offset, size);
    return 0;
  }
  
  return -1;
}

//---------------------------------------------------------------------------
int32 RamFsInode::writeData(int32 offset, int32 size, char *buffer)
{
  if(i_mode_ == I_FILE)
  {
    char *ptr_offset = data_ + offset;
    memcpy(ptr_offset, buffer, size);
    return 0;
  }
  
  return -1;
}

//---------------------------------------------------------------------------
int32 RamFsInode::mknod(Dentry *dentry)
{
  if(dentry == 0)
  {
    // ERROR_DNE
    return -1;
  }

  if(i_mode_ != I_DIR)
  {
    // ERROR_IC
    return -1;
  }
  
  i_dentry_ = dentry;
  dentry->setInode(this);
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
    i_dentry_link_.pushBack(dentry);
    dentry->setInode(this);
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
    if(i_dentry_link_.included(dentry) == false)
    {
      // ERROR_DNEILL
      return -1;
    }

    i_nlink_--;
    i_dentry_link_.remove(dentry);
 
    if(i_dentry_link_.empty() == false)
      return 0;
    else
      return INODE_DEAD;
  }
  else
  {
    // ERROR_HLI
    return -1;
  }

  // remove dentry
  dentry->releaseInode();
  Dentry *parent_dentry = dentry->getParent();
  parent_dentry->childRemove(dentry);
  delete dentry;

  return 0;
}

//---------------------------------------------------------------------------
int32 RamFsInode::rmdir()
{
  Dentry* dentry = i_dentry_;

  if(dentry->emptyChild() == true)
  {
    dentry->releaseInode();
    Dentry* parent_dentry = dentry->getParent();
    parent_dentry->childRemove(dentry);
    delete dentry;
    i_dentry_ = 0;
    return INODE_DEAD;
  }
  else
  {
    // ERROR_DEC
    return -1;
  }
}

//---------------------------------------------------------------------------
Dentry* RamFsInode::lookup(const char* name)
{
  if(name == 0)
  {
    // ERROR_DNE
    return 0;
  }
  
  Dentry* dentry_update = 0;
  if(i_mode_ == I_DIR)
  {
    dentry_update = i_dentry_->checkName(name);
    if(dentry_update == 0)
    {
      // ERROR_NNE
      return (Dentry*)0;
    }
    else
    {
      return dentry_update;
    }
  }
  else
  {
    // ERROR_IC
    return (Dentry*)0;
  }
}
