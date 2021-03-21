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
  Inode *root_inode = createInode(I_DIR);
  Dentry *root_dentry = new Dentry(root_inode);

  // mount the superblock over s_root or over default mount point

  if (s_root)
    mounted_over_ = s_root;
  else
    mounted_over_ = root_dentry;

  // create the inode for the root_dentry
  int32 root_init = root_inode->mknod(root_dentry);
  assert(root_init == 0);

  auto device_root_inode = createInode(I_DIR);
  Dentry *device_root_dentry = new Dentry(device_root_inode, root_dentry, DEVICE_ROOT_NAME);

  // create the inode for the device_root_dentry
  root_init = device_root_inode->mknod(device_root_dentry);
  assert(root_init == 0);

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
  Dentry* fdntr = new Dentry(device, s_dev_dentry_, device_name);

  cDevice = (Inode *) device;
  cDevice->mknod(fdntr);
  cDevice->setSuperBlock(this);

  all_inodes_.push_back(cDevice);
}

Inode* DeviceFSSuperBlock::createInode(uint32 type)
{
    auto inode = new RamFSInode(this, type);
    assert(inode);

    all_inodes_.push_back(inode);
    return inode;
}

int32 DeviceFSSuperBlock::createFd(Inode* inode, uint32 flag)
{
  assert(inode);

  File* file = inode->open(flag);
  FileDescriptor* fd = new FileDescriptor(file);
  s_files_.push_back(fd);
  FileDescriptor::add(fd);

  if (ustl::find(used_inodes_, inode) == used_inodes_.end())
  {
    used_inodes_.push_back(inode);
  }

  return (fd->getFd());
}


int32 DeviceFSSuperBlock::removeFd(Inode* inode, FileDescriptor* fd)
{
  assert(inode);
  assert(fd);

  s_files_.remove(fd);
  FileDescriptor::remove(fd);

  File* file = fd->getFile();
  int32 tmp = inode->release(file);

  debug(RAMFS, "remove the fd num: %d\n", fd->getFd());
  if (inode->getNumOpenedFile() == 0)
  {
    used_inodes_.remove(inode);
  }
  delete fd;

  return tmp;
}
