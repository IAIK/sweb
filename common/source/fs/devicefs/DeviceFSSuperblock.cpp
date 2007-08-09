#include "fs/devicefs/DeviceFSSuperblock.h"
#include "fs/ramfs/RamFsInode.h"
#include "fs/Dentry.h"
#include "fs/Inode.h"
#include "fs/File.h"
#include "fs/FileDescriptor.h"
#include "fs_global.h" 

#include "console/kprintf.h"

#include "Console.h"

extern Console* main_console;

const char DeviceFSSuperBlock::ROOT_NAME[] = { '/', 0 };

const char DeviceFSSuperBlock::DEVICE_ROOT_NAME[] = { 'd','e','v',0 };

DeviceFSSuperBlock* DeviceFSSuperBlock::instance_ = 0;

//----------------------------------------------------------------------
DeviceFSSuperBlock::DeviceFSSuperBlock(Dentry* s_root, uint32 s_dev) : Superblock(s_root, s_dev)
{ 
  // mount the superblock over s_root or over default mount point
  Dentry *root_dentry = new Dentry( ROOT_NAME );
  
  if ( s_root )
    mounted_over_ = s_root;
  else
    mounted_over_ = root_dentry;
  
  // create the inode for the root_dentry
  Inode *root_inode = (Inode*)(new RamFsInode(this, I_DIR));
  int32 root_init = root_inode->mknod( root_dentry );
  assert(root_init == 0);
  all_inodes_.pushBack( root_inode );
  
  Dentry *device_root_dentry = new Dentry( root_dentry );
  device_root_dentry->setName( DEVICE_ROOT_NAME );  
  
  // create the inode for the device_root_dentry
  Inode *device_root_inode = (Inode*)(new RamFsInode(this, I_DIR));
  root_init = device_root_inode->mknod( device_root_dentry );
  assert(root_init == 0);  
  all_inodes_.pushBack( device_root_inode );
  
  // set the root to / 
  s_root_ = root_dentry;
  // set the dev directory to /dev/ 
  s_dev_dentry_  = device_root_dentry;
  
  // load up the devices and register them
  
//   ///////////////////////////////////////////////
//   //  load character devices
//   ///////////////////////////////////////////////
//   
//   // load terminals
//   cDevice = 0;
//   uint32 term_num = 0;
//   uint32 num_terminals = main_console->getNumTerminals();
//   
//   for (; term_num < num_terminals; term_num++)
//   {
//     char terminal_name[] = { 't', 'e', 'r', 'm', 'X', 'X', 0 };
//     terminal_name[4] = (term_num / 10) + '0';
//     terminal_name[5] = (term_num % 10) + '0';
//     
// 
//   }
  
  // load serial devices
  cDevice = 0;
  
  ///////////////////////////////////////////////
  // load block devices
  ///////////////////////////////////////////////
  
  cDevice = 0;
}

//----------------------------------------------------------------------
DeviceFSSuperBlock::~DeviceFSSuperBlock()
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
//----------------------------------------------------------------------
//----------------------------------------------------------------------
void DeviceFSSuperBlock::addDevice( Inode* device, char* device_name )
{
  Dentry* fdntr = new Dentry( s_dev_dentry_ );
  fdntr->setName( device_name );

  cDevice = (Inode *) device;
  cDevice->mknod( fdntr );
  cDevice->setSuperBlock( this );

  all_inodes_.pushBack( cDevice );
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
Inode* DeviceFSSuperBlock::createInode(Dentry* dentry, uint32 type)
{
  Inode *inode = (Inode*)(new RamFsInode(this, type));
  assert(inode);
  if(type == I_DIR)
  {
    kprintfd("createInode: I_DIR\n");
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
int32 DeviceFSSuperBlock::createFd(Inode* inode, uint32 flag)
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
