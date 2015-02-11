#include "fs/ramfs/RamFSInode.h"
#include "kstring.h"
#include "assert.h"
#include "fs/ramfs/RamFSSuperblock.h"
#include "fs/ramfs/RamFSFile.h"
#include "fs/Dentry.h"

#include "console/kprintf.h"

#define BASIC_ALLOC 256

RamFSInode::RamFSInode(Superblock *super_block, uint32 inode_type) :
    Inode(super_block, inode_type), data_(0)
{
  if (inode_type == I_FILE)
    data_ = new char[BASIC_ALLOC];

  i_size_ = BASIC_ALLOC;
  i_nlink_ = 0;
  i_dentry_ = 0;
}

RamFSInode::~RamFSInode()
{
  delete[] data_;
}

int32 RamFSInode::readData(uint32 offset, uint32 size, char *buffer)
{
  if ((size + offset) > BASIC_ALLOC)
  {
    kprintfd("RamFSInode::ERROR: the size is bigger than size of the file\n");
    assert(true);
  }

  char *ptr_offset = data_ + offset;
  memcpy(buffer, ptr_offset, size);
  return size;
}

int32 RamFSInode::writeData(uint32 offset, uint32 size, const char *buffer)
{
  if ((size + offset) > BASIC_ALLOC)
  {
    kprintfd("RamFSInode::ERROR: the size is bigger than size of the file\n");
    assert(true);
  }

  assert(i_type_ == I_FILE);

  char *ptr_offset = data_ + offset;
  memcpy(ptr_offset, buffer, size);
  return size;
}

int32 RamFSInode::mknod(Dentry *dentry)
{
  if (dentry == 0)
  {
    // ERROR_DNE
    return -1;
  }

  if (i_type_ != I_DIR)
  {
    // ERROR_IC
    return -1;
  }

  i_dentry_ = dentry;
  dentry->setInode(this);
  return 0;
}

int32 RamFSInode::mkdir(Dentry *dentry)
{
  return (mkdir(dentry));
}

int32 RamFSInode::mkfile(Dentry *dentry)
{
  if (dentry == 0)
  {
    // ERROR_DNE
    return -1;
  }

  if (i_type_ != I_FILE)
  {
    // ERROR_IC
    return -1;
  }

  i_dentry_ = dentry;
  dentry->setInode(this);
  return 0;
}

int32 RamFSInode::create(Dentry *dentry)
{
  return (mkdir(dentry));
}

File* RamFSInode::link(uint32 flag)
{
  File* file = (File*) (new RamFSFile(this, i_dentry_, flag));
  i_files_.push_back(file);
  return file;
}

int32 RamFSInode::unlink(File* file)
{
  i_files_.remove(file);
  delete file;
  return 0;
}

int32 RamFSInode::rmdir()
{
  if (i_type_ != I_DIR)
    return -1;

  Dentry* dentry = i_dentry_;

  if (dentry->emptyChild() == true)
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

int32 RamFSInode::rm()
{
  if (i_files_.size() != 0)
  {
    kprintfd("RamFSInode::ERROR: the file is opened.\n");
    return -1;
  }

  Dentry* dentry = i_dentry_;

  if (dentry->emptyChild() == true)
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

Dentry* RamFSInode::lookup(const char* name)
{
  if (name == 0)
  {
    // ERROR_DNE
    return 0;
  }

  Dentry* dentry_update = 0;
  if (i_type_ == I_DIR)
  {
    dentry_update = i_dentry_->checkName(name);
    if (dentry_update == 0)
    {
      // ERROR_NNE
      return (Dentry*) 0;
    }
    else
    {
      return dentry_update;
    }
  }
  else
  {
    // ERROR_IC
    return (Dentry*) 0;
  }
}

