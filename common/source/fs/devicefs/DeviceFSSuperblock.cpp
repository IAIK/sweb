/**
 * @file DeviceFSSuperblock.cpp
 */

#include "fs/devicefs/DeviceFSSuperblock.h"
#include "fs/ramfs/RamFSInode.h"
#include "fs/Dentry.h"
#include "fs/Inode.h"
#include "fs/File.h"
#include "fs/FileDescriptor.h"

#include "console/kprintf.h"

#include "Console.h"

extern Console* main_console;

const char DeviceFSSuperBlock::ROOT_NAME[] = "/";
const char DeviceFSSuperBlock::DEVICE_ROOT_NAME[] = "dev";

DeviceFSSuperBlock* DeviceFSSuperBlock::instance_ = 0;

DeviceFSSuperBlock::DeviceFSSuperBlock(Dentry* s_root, uint32 s_dev) :
    Superblock(s_root, s_dev)
{
  // mount the superblock over s_root or over default mount point
  Dentry *root_dentry = new Dentry(ROOT_NAME);

  if (s_root)
    mounted_over_ = s_root;
  else
    mounted_over_ = root_dentry;

  // create the inode for the root_dentry
  Inode *root_inode = (Inode*) (new RamFSInode(this, I_DIR));
  int32 root_init = root_inode->mknod(root_dentry);
  assert(root_init == 0);
  all_inodes_.push_back(root_inode);

  Dentry *device_root_dentry = new Dentry(root_dentry);
  device_root_dentry->d_name_ = DEVICE_ROOT_NAME;

  // create the inode for the device_root_dentry
  Inode *device_root_inode = (Inode*) (new RamFSInode(this, I_DIR));
  root_init = device_root_inode->mknod(device_root_dentry);
  assert(root_init == 0);
  all_inodes_.push_back(device_root_inode);

  // set the root to /
  s_root_ = root_dentry;
  // set the dev directory to /dev/
  s_dev_dentry_ = device_root_dentry;

  cDevice = 0;
}

DeviceFSSuperBlock::~DeviceFSSuperBlock()
{
  assert(dirty_inodes_.empty() == true);

  for (FileDescriptor* fd : s_files_)
  {
    delete fd->getFile();
    delete fd;
  }
  s_files_.clear();

  for (Inode* inode : all_inodes_)
  {
    delete inode->getDentry();
    delete inode;
  }
  all_inodes_.clear();
}

void DeviceFSSuperBlock::addDevice(Inode* device, const char* device_name)
{
  Dentry* fdntr = new Dentry(s_dev_dentry_);
  fdntr->d_name_ = device_name;

  cDevice = (Inode *) device;
  cDevice->mknod(fdntr);
  cDevice->setSuperBlock(this);

  all_inodes_.push_back(cDevice);
}

Inode* DeviceFSSuperBlock::createInode(Dentry* dentry, uint32 type)
{
  Inode *inode = (Inode*) (new RamFSInode(this, type));
  assert(inode);
  if (type == I_DIR)
  {
    //kprintfd ( "createInode: I_DIR\n" );
    int32 inode_init = inode->mknod(dentry);
    assert(inode_init == 0);
  }
  else if (type == I_FILE)
  {
    kprintfd("createInode: I_FILE\n");
    int32 inode_init = inode->mkfile(dentry);
    assert(inode_init == 0);
  }

  all_inodes_.push_back(inode);
  return inode;
}

int32 DeviceFSSuperBlock::createFd(Inode* inode, uint32 flag)
{
  assert(inode);

  File* file = inode->link(flag);
  FileDescriptor* fd = new FileDescriptor(file);
  s_files_.push_back(fd);
  FileDescriptor::add(fd);

  if (ustl::find(used_inodes_, inode) == used_inodes_.end())
  {
    used_inodes_.push_back(inode);
  }

  return (fd->getFd());
}
