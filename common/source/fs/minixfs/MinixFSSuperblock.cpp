// Projectname: SWEB
// Simple operating system for educational purposes

#include "fs/FileDescriptor.h"
#include "fs/minixfs/MinixFSSuperblock.h"
#include "fs/minixfs/MinixFSInode.h"
#include "fs/minixfs/MinixFSFile.h"
#include "fs/Dentry.h"
#include "fs_global.h"
#include "assert.h"
#include "arch_bd_manager.h"
#include "kmalloc.h"

#include "console/kprintf.h"
#define ROOT_NAME "/"
#define BLOCK_SIZE 1024

uint16 MinixFSSuperblock::readBytes(char* buffer, uint32 offset)
{
  uint16 dst = 0;
  dst |= buffer[offset + 1];
  dst = dst << 8;
  dst |= (buffer[offset] & 0xFF);
  return dst;
}


//----------------------------------------------------------------------
MinixFSSuperblock::MinixFSSuperblock(Dentry* s_root, uint32 s_dev) : Superblock(s_root, s_dev)
{
  
  kprintfd("------------dev id: %d", s_dev);
  //TODO:read Superblock data from disc
  char *buffer = new char[BLOCK_SIZE*sizeof(uint8)];
  //char *buffer = (char *) kmalloc( BLOCK_SIZE*sizeof(uint8) );
  BDRequest * bd = new BDRequest(s_dev_, BDRequest::BD_READ, 2, 1, buffer);
  BDManager::getInstance()->getDeviceByNumber(3)->addRequest ( bd );
  uint32 jiffies = 0;
  while( bd->getStatus() == BDRequest::BD_QUEUED && jiffies++ < 50000 );
  if( bd->getStatus() == BDRequest::BD_DONE )
  {
    s_num_inodes_ = readBytes(buffer, 0);
    s_num_zones_ = readBytes(buffer, 2);
    s_num_inode_bm_blocks_ = readBytes(buffer, 4);
    s_num_zone_bm_blocks_ = readBytes(buffer, 6);
    s_1st_datazone_ = readBytes(buffer, 8);
    s_log_zone_size_ = readBytes(buffer, 10);
    s_max_file_size_ = readBytes(buffer, 14);
    s_max_file_size_ = s_max_file_size_ << 16;
    s_max_file_size_ |= readBytes(buffer, 12);
    s_magic_ = readBytes(buffer, 16);
    kprintfd("------------DONE\n");
  }
  kprintfd("s_num_inodes_ : %d\ns_num_zones_ : %d\ns_num_inode_bm_blocks_ : %d\ns_num_zone_bm_blocks_ : %d\ns_1st_datazone_ : %d\ns_log_zone_size_ : %d\ns_max_file_size_ : %d\ns_magic_ : %d\n",s_num_inodes_,s_num_zones_,s_num_inode_bm_blocks_,s_num_zone_bm_blocks_,s_1st_datazone_,s_log_zone_size_,s_max_file_size_,s_magic_);
  
  for(uint32 i = 0; i < BLOCK_SIZE; i++ )
    kprintfd( "%2X%c", *(buffer+i), i%8 ? ' ' : '\n' );
  delete[] buffer;
  
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
  //TODO:init Storage Manager
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
