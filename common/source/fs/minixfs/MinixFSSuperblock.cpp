// Projectname: SWEB
// Simple operating system for educational purposes

#include "fs/FileDescriptor.h"
#include "fs/minixfs/MinixFSSuperblock.h"
#include "fs/minixfs/MinixFSInode.h"
#include "fs/minixfs/MinixFSFile.h"
#include "fs/Dentry.h"
#include "fs_global.h"
#include "assert.h"

#include "console/kprintf.h"
#define ROOT_NAME "/"

//----------------------------------------------------------------------
MinixFSSuperblock::MinixFSSuperblock(Dentry* s_root) : Superblock(s_root)
{
  Dentry *root_dentry = new Dentry(ROOT_NAME);

  if (s_root)
  {
    // MOUNT
    mounted_over_ = s_root;
  }
  else
  {
    // ROOT
    mounted_over_ = root_dentry;
  }
  s_root_ = root_dentry;

  // create the inode for the root_dentry
  Inode *root_inode = (Inode*)(new MinixFSInode(this, I_DIR));
  int32 root_init = root_inode->mknod(root_dentry);
  assert(root_init == 0);

  // add the root_inode in the list
  all_inodes_.pushBack(root_inode);
  //TODO:implement and call initInodes();
}

//----------------------------------------------------------------------
MinixFSSuperblock::~MinixFSSuperblock()
{
  assert(dirty_inodes_.empty() == true);

  uint32 num = s_files_.getLength();
  for(uint32 counter = 0; counter < num; counter++)
  {
    FileDescriptor *fd = s_files_.at(0);
    File* file = fd->getFile();
    s_files_.remove(fd);

    if(file)
    {
      delete file;
    }
    delete fd;
  }

  assert(s_files_.empty() == true);

  num = all_inodes_.getLength();
  for(uint32 counter = 0; counter < num; counter++)
  {
    Inode* inode = all_inodes_.at(0);
    all_inodes_.remove(inode);
    Dentry* dentry = inode->getDentry();
    delete dentry;

    delete inode;
  }

  assert(all_inodes_.empty() == true);
}

//----------------------------------------------------------------------
Inode* MinixFSSuperblock::createInode(Dentry* dentry, uint32 type)
{
  Inode *inode = (Inode*)(new MinixFSInode(this, type));
  if(type == I_DIR)
  {
    int32 inode_init = inode->mknod(dentry);
    assert(inode_init == 0);
  }
  else if(type == I_FILE)
  {
    kprintfd("createInode: I_FILE\n");
    int32 inode_init = inode->mkfile(dentry);
    assert(inode_init == 0);
  }

  all_inodes_.pushBack(inode);
  return inode;
}

//----------------------------------------------------------------------
int32 MinixFSSuperblock::readInode(Inode* inode)
{
  assert(inode);

  if (!all_inodes_.included(inode))
  {
    all_inodes_.pushBack(inode);
  }
  return 0;
}

//----------------------------------------------------------------------
void MinixFSSuperblock::write_inode(Inode* inode)
{
  assert(inode);

  if (!all_inodes_.included(inode))
  {
    all_inodes_.pushBack(inode);
  }
}

//----------------------------------------------------------------------
void MinixFSSuperblock::delete_inode(Inode* inode)
{
  all_inodes_.remove(inode);
  //TODO delete(inode); ?
}

//----------------------------------------------------------------------
int32 MinixFSSuperblock::createFd(Inode* inode, uint32 flag)
{
  assert(inode);

  File* file = inode->link(flag);
  FileDescriptor* fd = new FileDescriptor(file);
  s_files_.pushBack(fd);
  global_fd.pushBack(fd);

  if (!used_inodes_.included(inode))
  {
    used_inodes_.pushBack(inode);
  }

  return(fd->getFd());
}

//----------------------------------------------------------------------
int32 MinixFSSuperblock::removeFd(Inode* inode, FileDescriptor* fd)
{
  assert(inode);
  assert(fd);

  s_files_.remove(fd);
  //global_fd.remove(fd);

  File* file = fd->getFile();
  int32 tmp = inode->unlink(file);

  kprintfd("remove the fd num: %d\n", fd->getFd());
  if(inode->getNumOpenedFile() == 0)
  {
    used_inodes_.remove(inode);
  }
  delete fd;

  return tmp;
}


//----------------------------------------------------------------------
int32 MinixFSSuperblock::initInodes()
{
  //TODO:
  // get the start pointer of the disc - or guess wildly
  // read the superblock data
  // read the inode bitmap
  // create a table to map the inode bitmap to inode pointer
  // read and create the inodes form disc which are marked used in the bitmap
  assert(false);
  return 0;
}
