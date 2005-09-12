#include "fs/VfsSyscall.h"
#include "util/string.h"
#include "assert.h"
#include "fs/fs_global.h"
#include "mm/kmalloc.h"
#include "fs/Dirent.h"

#include "fs/Inode.h"
#include "fs/Dentry.h"
#include "fs/Superblock.h"

#include "console/kprintf.h"

VfsSyscall vfs_syscall;

//---------------------------------------------------------------------------
int32 VfsSyscall::mkdir(char* new_dir, int32 /*mode*/)
{
  kprintfd("***** start of syscall mkdir()\n");
  if(new_dir == 0)
    return -1;

  char* tmp = 0;
  char* ptr = new_dir;
  
  for(;;) // found the last slash
  {
    while(*ptr == '/')
      ptr++;

    tmp = strchr(ptr, '/');
    if(tmp == 0)
      break;
    
    ptr = tmp;
  }
  
  assert(ptr != 0);
  
  kprintfd("set directory\n");
  uint32 path_len = (ptr - 1) - new_dir + 1; 
    // (ptr - 1) becauce the ptr++ gots to the next character

  fs_info.setName(new_dir, path_len);
  
  char* test_name = fs_info.getName();
  kprintfd("test_name_len = %s, has length %d\n", test_name, strlen(test_name));

  uint32 dir_name_len = strlen(new_dir) - (path_len - 1) + 1;
  char* dir_name = (char*)kmalloc(dir_name_len * sizeof(char));
  strlcpy(dir_name, ptr, dir_name_len);
  kprintfd("dir_name = %s, has length %d\n", dir_name, strlen(dir_name));

  // fs_info.getName() = parent directory
  // dir_name is the new directory, that will be to create

  int32 success = path_walker.pathInit(fs_info.getName(), 0);
  kprintfd("after pathInit() success = %d\n", success);
  if(success == 0)
    success = path_walker.pathWalk(fs_info.getName());
  kprintfd("after pathWalk() success = %d\n", success);

  kprintfd("get the dentry of the path_walker\n");
  Dentry* current_dentry = path_walker.getDentry();
  Inode* current_inode = current_dentry->getInode();
  Superblock *current_sb = current_inode->getSuperblock();
  
  if(current_inode->getMode() != I_DIR)
  {
    kprintfd("This file is not a directory");
    kfree(dir_name);
    path_walker.pathRelease();
    fs_info.putName();
    return -1;
  }
  
  kprintfd("create a new dentry\n");
  Dentry* sub_dentry = new Dentry(current_dentry);
  sub_dentry->setName(dir_name);
  kprintfd("number of the child: %d\n", current_dentry->getNumChild());
  current_sb->createInode(sub_dentry, I_DIR);

  kfree(dir_name);
  path_walker.pathRelease();
  fs_info.putName();
 
  kprintfd("***** end of syscall mkdir()\n");
  
  return 0;
}

//---------------------------------------------------------------------------
Dirent* VfsSyscall::readdir(char* dir)
{
  kprintfd("***** start of syscall readdir()\n");

  if(dir == 0)
    return (Dirent*)0;

  fs_info.setName(dir);
  
  char* test_name = fs_info.getName();
  kprintfd("test_name_len = %s, has length %d\n", test_name, strlen(test_name));

  // fs_info.getName() = directory

  int32 success = path_walker.pathInit(fs_info.getName(), 0);
  kprintfd("after pathInit() success = %d\n", success);
  if(success == 0)
    success = path_walker.pathWalk(fs_info.getName());
  kprintfd("after pathWalk() success = %d\n", success);

  kprintfd("get the dentry of the path_walker\n");
  Dentry* current_dentry = path_walker.getDentry();
  Inode* current_inode = current_dentry->getInode();

  if(current_inode->getMode() != I_DIR)
  {
    kprintfd("This file is not a directory");
    path_walker.pathRelease();
    fs_info.putName();
    return (Dirent*)0;
  }

  for(uint32 counter = 0; counter < current_dentry->getNumChild(); counter++)
  {
    Dentry* sub_dentry = current_dentry->getChild(counter);
    Inode* sub_inode = sub_dentry->getInode();
    uint32 inode_mode = sub_inode->getMode();
    
    switch(inode_mode)
    {
      case I_DIR:
        kprintfd("[D]");
        break;
      case I_FILE:
        kprintfd("[F]");
        break;
      case I_LNK:
        kprintfd("[L]");
        break;
      default:
        break;
    }
    
    kprintfd("%s\n", sub_dentry->getName());
  }

  path_walker.pathRelease();
  fs_info.putName();
 
  kprintfd("***** end of syscall readdir()\n");
  
  return((Dirent*)0);
}

//---------------------------------------------------------------------------
int32 VfsSyscall::chdir(char* dir)
{
  kprintfd("***** start of syscall chdir()\n");
  if(dir == 0)
    return -1;

  fs_info.setName(dir);
  
  char* test_name = fs_info.getName();
  kprintfd("test_name_len = %s, has length %d\n", test_name, strlen(test_name));

  // fs_info.getName() = directory

  int32 success = path_walker.pathInit(fs_info.getName(), 0);
  kprintfd("after pathInit() success = %d\n", success);
  if(success == 0)
    success = path_walker.pathWalk(fs_info.getName());
  kprintfd("after pathWalk() success = %d\n", success);

  kprintfd("get the dentry of the path_walker\n");
  Dentry* current_dentry = path_walker.getDentry();
  Inode* current_inode = current_dentry->getInode();

  if(current_inode->getMode() != I_DIR)
  {
    kprintfd("This file is not a directory");
    path_walker.pathRelease();
    fs_info.putName();
    return -1;
  }

  fs_info.setFsPwd(path_walker.getDentry(), path_walker.getVfsMount());

  path_walker.pathRelease();
  fs_info.putName();
 
  kprintfd("***** end of syscall chddir()\n");
  
  return 0;
}

//---------------------------------------------------------------------------
int32 VfsSyscall::rmdir(char* dir)
{
  kprintfd("***** start of syscall rmdir()\n");
  if(dir == 0)
    return -1;

  fs_info.setName(dir);
  
  char* test_name = fs_info.getName();
  kprintfd("test_name_len = %s, has length %d\n", test_name, strlen(test_name));

  // fs_info.getName() = directory

  int32 success = path_walker.pathInit(fs_info.getName(), 0);
  kprintfd("after pathInit() success = %d\n", success);
  if(success == 0)
    success = path_walker.pathWalk(fs_info.getName());
  kprintfd("after pathWalk() success = %d\n", success);

  kprintfd("get the dentry of the path_walker\n");
  Dentry* current_dentry = path_walker.getDentry();
  Inode* current_inode = current_dentry->getInode();
  if(current_inode->getMode() != I_DIR)
  {
    kprintfd("This file is not a directory\n");
    path_walker.pathRelease();
    fs_info.putName();
    return -1;
  }

  Superblock* sb = current_inode->getSuperblock();
  kprintfd("~~~~ rmdir()\n");
  if(current_inode->rmdir() == INODE_DEAD)
  {
    kprintfd("remove the inode from the list\n");
    sb->delete_inode(current_inode);
  }
  else
  {
    kprintfd("remove the inode failed\n");
    path_walker.pathRelease();
    fs_info.putName();
    return -1;
  }
  
  path_walker.pathRelease();
  fs_info.putName();
 
  kprintfd("***** end of syscall rmdir()\n");
  
  return 0;
}
