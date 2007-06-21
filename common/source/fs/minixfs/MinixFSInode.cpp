
#include "fs/minixfs/MinixFSInode.h"

#include "mm/kmalloc.h"
#include "util/string.h"
#include "assert.h"
#include "fs/minixfs/MinixFSSuperblock.h"
#include "fs/minixfs/MinixFSFile.h"
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
MinixFSInode::MinixFSInode(Superblock *super_block, uint32 inode_type) :
    Inode(super_block, inode_type)
{
  if(inode_type == I_FILE)
    data_ = (char*)i_superblock_->getStorageManager()->allocateMemory(BASIC_ALLOC);
  else
    data_ = 0;

  i_size_ = BASIC_ALLOC;
  i_nlink_ = 0;
  i_dentry_ = 0;
}

//---------------------------------------------------------------------------
MinixFSInode::~MinixFSInode()
{
  if (data_)
  {
    i_superblock_->getStorageManager()->freeMemory(data_);
  }
}

//---------------------------------------------------------------------------
//TODO Files could be bigger than the basic allocated file
//TODO if there is less to read than requested return less.
//NOTE maybe pointer has to be translated?
int32 MinixFSInode::readData(int32 offset, int32 size, char *buffer)
{
  if((size + offset) > BASIC_ALLOC)
  {
    kprintfd("the size is bigger than size of the file\n");
    assert(false);
  }

  char *ptr_offset = data_ + offset;
  memcpy(buffer, ptr_offset, size);
  return size;
}
//TODO reallocate memory if writing beyond file size
// eventually change fd to new address
//NOTE maybe pointer has to be translated?
//---------------------------------------------------------------------------
int32 MinixFSInode::writeData(int32 offset, int32 size, const char *buffer)
{
  if((size + offset) > BASIC_ALLOC)
  {
    kprintfd("the size is bigger than size of the file\n");
    assert(true);
  }

  assert(i_type_ == I_FILE);

  char *ptr_offset = data_ + offset;
  memcpy(ptr_offset, buffer, size);
  return size;
}

//---------------------------------------------------------------------------
int32 MinixFSInode::mknod(Dentry *dentry)
{
  if(dentry == 0)
  {
    // ERROR_DNE
    return -1;
  }

  if(i_type_ != I_DIR)
  {
    // ERROR_IC
    return -1;
  }

  i_dentry_ = dentry;
  dentry->setInode(this);
  return 0;
}

//---------------------------------------------------------------------------
int32 MinixFSInode::mkdir(Dentry *dentry)
{
  //TODO implement
  assert(false);
  return(mkdir(dentry));
}

//---------------------------------------------------------------------------
int32 MinixFSInode::mkfile(Dentry *dentry)
{
  if(dentry == 0)
  {
    // ERROR_DNE
    return -1;
  }

  if(i_type_ != I_FILE)
  {
    // ERROR_IC
    return -1;
  }

  i_dentry_ = dentry;
  dentry->setInode(this);
  return 0;
}

//---------------------------------------------------------------------------
int32 MinixFSInode::create(Dentry *dentry)
{
  //TODO implement
  assert(false);
  return(mkdir(dentry));
}

//---------------------------------------------------------------------------
File* MinixFSInode::link(uint32 flag)
{
  File* file = (File*)(new MinixFSFile(this, i_dentry_, flag));
  i_files_.pushBack(file);
  return file;
}

/*
  if(dentry == 0)
{
    // ERROR_DNE
    return -1;
}

  if(i_type_ == I_FILE)
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
*/

//---------------------------------------------------------------------------
int32 MinixFSInode::unlink(File* file)
{
  int32 tmp = i_files_.remove(file);
  delete file;
  return tmp;
}
  /*
  if(dentry == 0)
{
    // ERROR_DNE
    return -1;
}

  if(i_type_ == I_FILE)
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
  */

//---------------------------------------------------------------------------
int32 MinixFSInode::rmdir()
{
  if(i_type_ != I_DIR)
    return -1;

  Dentry* dentry = i_dentry_;

  //NOTE empty instead of emptyChild?
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
int32 MinixFSInode::rm()
{
  if(i_files_.getLength() != 0)
  {
    kprintfd("the file is opened.\n");
    return -1;
  }

  Dentry* dentry = i_dentry_;

  //NOTE empty instead of emptyChild?
  //NOTE has to be file so should not have children
  if(dentry->emptyChild() == true)
  {
    dentry->releaseInode();
    Dentry* parent_dentry = dentry->getParent();
  //NOTE if it is possible to be a directory it has to be deleted recursively
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
Dentry* MinixFSInode::lookup(const char* name)
{
  if(name == 0)
  {
    // ERROR_DNE
    return 0;
  }

  Dentry* dentry_update = 0;
  if(i_type_ == I_DIR)
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

