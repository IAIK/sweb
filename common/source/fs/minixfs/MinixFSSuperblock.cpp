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
#define INODES_PER_BLOCK 32


//----------------------------------------------------------------------
MinixFSSuperblock::MinixFSSuperblock(Dentry* s_root, uint32 s_dev) : Superblock(s_root, s_dev)
{
  //read Superblock data from disc
  char *buffer = new char[BLOCK_SIZE*sizeof(uint8)];
  BDRequest * bd = new BDRequest(s_dev_, BDRequest::BD_READ, 2, 1, buffer);
  BDManager::getInstance()->getDeviceByNumber(s_dev)->addRequest ( bd );
  uint32 jiffies = 0;
  while( bd->getStatus() == BDRequest::BD_QUEUED && jiffies++ < 50000 );
  if( bd->getStatus() == BDRequest::BD_DONE )
  {
    s_num_inodes_ = read2Bytes(buffer, 0);
    s_num_zones_ = read2Bytes(buffer, 2);
    s_num_inode_bm_blocks_ = read2Bytes(buffer, 4);
    s_num_zone_bm_blocks_ = read2Bytes(buffer, 6);
    s_1st_datazone_ = read2Bytes(buffer, 8);
    s_log_zone_size_ = read2Bytes(buffer, 10);
    s_max_file_size_ = read4Bytes(buffer, 12);
    s_magic_ = read2Bytes(buffer, 16);
  }
  else
  {
    assert(0);
  }
  kprintfd("s_num_inodes_ : %d\ns_num_zones_ : %d\ns_num_inode_bm_blocks_ : %d\ns_num_zone_bm_blocks_ : %d\ns_1st_datazone_ : %d\ns_log_zone_size_ : %d\ns_max_file_size_ : %d\ns_magic_ : %d\n",s_num_inodes_,s_num_zones_,s_num_inode_bm_blocks_,s_num_zone_bm_blocks_,s_1st_datazone_,s_log_zone_size_,s_max_file_size_,s_magic_);
  
//   for(uint32 i = 0; i < BLOCK_SIZE; i++ )
//     kprintfd( "%2X%c", *(buffer+i), i%8 ? ' ' : '\n' );

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

  //create Storage Manager
  uint32 bm_size = s_num_inode_bm_blocks_ + s_num_zone_bm_blocks_;
  char *bm_buffer = new char[BLOCK_SIZE*sizeof(uint8)*bm_size];
  BDRequest * bm_bd = new BDRequest(s_dev_, BDRequest::BD_READ, 3, bm_size, bm_buffer);
  BDManager::getInstance()->getDeviceByNumber(s_dev)->addRequest ( bm_bd );
  jiffies = 0;
  while( bm_bd->getStatus() == BDRequest::BD_QUEUED && jiffies++ < 50000 );
  if( bm_bd->getStatus() == BDRequest::BD_DONE )
  {
    storage_manager_ = new MinixStorageManager(bm_buffer,
                                               s_num_inode_bm_blocks_,
                                               s_num_zone_bm_blocks_,
                                               s_num_inodes_, s_num_zones_);
  }
  else
  {
    assert(0);
  }
  delete[] bm_buffer;

  //call initInodes();
   initInodes();
   //TODO:
//   int32 root_init = all_inodes_.at(0)->mknod(s_root_);
//   assert(root_init == 0);
}


//----------------------------------------------------------------------
void MinixFSSuperblock::initInodes()
{
  // starts after the MBR, Superblock, InodeBM and ZoneBM. ends when the Data starts.
  uint32 inodes_start = s_num_inode_bm_blocks_ + s_num_zone_bm_blocks_ + 2;
  uint32 num_inode_blocks = s_1st_datazone_ - inodes_start;
  // read and create the inodes form disc which are marked used in the bitmap
  uint32 num_used_inodes = storage_manager_->getNumUsedInodes();
  uint32 inodes_read = 0;
  uint32 curr_inode = 0;
  uint32 offset = 0;
  char *buffer = new char[BLOCK_SIZE*sizeof(uint8)];
  for (uint32 block = 0; block < num_inode_blocks && inodes_read < num_used_inodes; block++)
  {
    BDRequest * bd = new BDRequest(s_dev_, BDRequest::BD_READ, inodes_start + block, 1, buffer);
    BDManager::getInstance()->getDeviceByNumber(s_dev_)->addRequest ( bd );
    uint32 jiffies = 0;
    while( bd->getStatus() == BDRequest::BD_QUEUED && jiffies++ < 50000 );
    if( bd->getStatus() == BDRequest::BD_DONE )
    {
      for(;curr_inode < INODES_PER_BLOCK * (block + 1); curr_inode++)
      {
        if(storage_manager_->isInodeSet(curr_inode))
        {
          uint16 *i_zones = new uint16[9];
          for(uint32 num_zone = 0; num_zone < 9; num_zone ++)
          {
            i_zones[num_zone] = read2Bytes(buffer, offset + 14 + (num_zone * 2));
          }
          offset = curr_inode - ( block * INODES_PER_BLOCK );
          all_inodes_.pushBack(new MinixFSInode( this,
                                               read2Bytes(buffer, offset),
                                               read2Bytes(buffer, offset + 2),
                                               read4Bytes(buffer, offset + 4),
                                               read4Bytes(buffer, offset + 8),
                                               read1Byte(buffer, offset + 12),
                                               read1Byte(buffer, offset + 13),
                                               i_zones
                                             )
                              );
        }
      }
    }
    else
    {
      assert(0);
    }
  }
  delete[] buffer;
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

uint8 read1Byte(char* buffer, uint32 offset)
{
  return buffer[offset];
}

uint16 MinixFSSuperblock::read2Bytes(char* buffer, uint32 offset)
{
  uint16 dst = 0;
  dst |= buffer[offset + 1];
  dst = dst << 8;
  dst |= (buffer[offset] & 0xFF);
  return dst;
}

uint32 MinixFSSuperblock::read4Bytes(char* buffer, uint32 offset)
{
  uint32 dst = 0;
  dst |= read2Bytes(buffer, offset + 2);
  dst = dst << 16;
  dst |= (read2Bytes(buffer, offset) & 0xFFFF);
  return dst;
}

