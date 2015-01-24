/**
 * @file VfsSyscall.cpp
 */
#include <iostream>

#include "VfsSyscall.h"
#include <assert.h>
#include "Inode.h"
#include "Dentry.h"
#include "Superblock.h"
#include "MinixFSSuperblock.h"
#include "File.h"
#include "FileDescriptor.h"
#include "FileSystemInfo.h"
#include "PathWalker.h"
#include "PointList.h"
#include "string.h"

extern FileSystemInfo *fs_info;
extern PointList<FileDescriptor> global_fd;

#define SEPARATOR '/'
#define CHAR_DOT '.'


VfsSyscall::VfsSyscall(uint32 dev, uint64 offset)
{
  superblock_ = new MinixFSSuperblock(0, dev, offset);
  Dentry *mount_point = superblock_->getMountPoint();
  mount_point->setMountPoint ( mount_point );
  Dentry *root = superblock_->getRoot();

  fs_info = new FileSystemInfo();
  fs_info->setFsRoot ( root);
  fs_info->setFsPwd ( root);
}

VfsSyscall::~VfsSyscall()
{
  delete superblock_;
  delete fs_info;
}

FileDescriptor* VfsSyscall::getFileDescriptor ( uint32 fd )
{
  FileDescriptor* file_descriptor = 0;
  uint32 num = global_fd.getLength();
  for ( uint32 counter = 0; counter < num; counter++ )
  {
    if ( global_fd.at ( counter )->getFd() == fd )
    {
      file_descriptor = global_fd.at ( counter );
      //debug ( VFSSYSCALL,"found the fd\n" );
      break;
    }
  }
  return file_descriptor;
}


int32 VfsSyscall::dupChecking ( const char* pathname )
{
  //FileSystemInfo *fs_info = currentThread->getFSInfo();
  if ( pathname == 0 )
    return -1;

  if ( ( pathname[0] != SEPARATOR ) && ( pathname[1] != SEPARATOR ) &&
          ( pathname[2] != SEPARATOR ) )
  {
    uint32 path_len = strlen ( pathname ) + 1;
    char *path_tmp = new char[path_len + 2];
    // path_tmp = "./" + pathname + '\0'
    char *path_tmp_ptr = path_tmp;
    *path_tmp_ptr++ = CHAR_DOT;
    *path_tmp_ptr++ = SEPARATOR;
    strlcpy ( path_tmp_ptr, pathname, path_len );
    fs_info->setName ( path_tmp );
    delete[] path_tmp;
  }
  else
    fs_info->setName ( pathname );

  int32 success = path_walker_.pathInit ( fs_info->getName(), 0 );
  if ( success == 0 )
    success = path_walker_.pathWalk ( fs_info->getName() );

  // checked
  return success;
}


int32 VfsSyscall::mkdir ( const char* pathname, int32 )
{
  //debug ( VFSSYSCALL,"(mkdir) \n" );
  //FileSystemInfo *fs_info = currentThread->getFSInfo();
  //std::cout << "mkdir called with pathname " << pathname << std::endl;
  if ( dupChecking ( pathname ) == 0 )
  {
    //debug ( VFSSYSCALL,"(mkdir) the pathname exists\n" );
    path_walker_.pathRelease();
    fs_info->putName();
    return -2;
  }
  //debug ( VFSSYSCALL,"(mkdir) pathRelease();\n" );
  path_walker_.pathRelease();
  char* path_tmp = new char[strlen ( fs_info->getName() ) + 1];
  strlcpy ( path_tmp, fs_info->getName(), ( strlen ( fs_info->getName() ) + 1 ) );
  fs_info->putName();

  char* char_tmp = strrchr ( path_tmp, SEPARATOR );
  assert ( char_tmp != 0 );
  //debug ( VFSSYSCALL,"(mkdir)setName \n" );
  // set directory
  uint32 path_prev_len = char_tmp - path_tmp + 1;
  fs_info->setName ( path_tmp, path_prev_len );

  char* path_prev_name = fs_info->getName();
  //debug ( VFSSYSCALL,"(mkdir) path_prev_name: %s\n",path_prev_name );
  int32 success = path_walker_.pathInit ( path_prev_name, 0 );
  if ( success == 0 )
    success = path_walker_.pathWalk ( path_prev_name );

  if ( success != 0 )
  {
    //debug ( VFSSYSCALL,"path_walker failed\n\n" );
    path_walker_.pathRelease();
    path_prev_name[path_prev_len - 1] = 0;

    if(mkdir(path_prev_name, 0) == -1)
    {
      delete[] path_tmp;
      delete[] path_prev_name;
      return -1;
    }

    int32 success2 = path_walker_.pathInit ( path_prev_name, 0 );
    if ( success2 == 0 )
      success2 = path_walker_.pathWalk ( path_prev_name );
    if ( success2 != 0 )
    {
      path_walker_.pathRelease();
      delete[] path_tmp;
      delete[] path_prev_name;  
      return -1;
    }
  }

  delete[] path_prev_name;

  //std::cout << "mkdir: pathwalk successful, creating dentry" << std::endl;
  Dentry* current_dentry = path_walker_.getDentry();
  path_walker_.pathRelease();
  Inode* current_inode = current_dentry->getInode();
  Superblock* current_sb = current_inode->getSuperblock();

  if ( current_inode->getType() != I_DIR )
  {
    //debug ( VFSSYSCALL,"This path is not a directory\n\n" );
    return -1;
  }

  char_tmp++;
  uint32 path_next_len = strlen ( path_tmp ) - path_prev_len + 1;
  char* path_next_name = new char[path_next_len];
  strlcpy ( path_next_name, char_tmp, path_next_len );

  // create a new dentry
  Dentry *sub_dentry = new Dentry ( current_dentry );
  sub_dentry->setName ( path_next_name );
  //sub_dentry->setParent ( current_dentry );
  delete[] path_next_name;
  delete[] path_tmp;
  //debug ( VFSSYSCALL,"(mkdir) creating Inode: current_dentry->getName(): %s\n",current_dentry->getName() );
  //debug ( VFSSYSCALL,"(mkdir) creating Inode: sub_dentry->getName(): %s\n",sub_dentry->getName() );
  //debug ( VFSSYSCALL,"(mkdir) current_sb: %d\n",current_sb );
  //debug ( VFSSYSCALL,"(mkdir) current_sb->getFSType(): %d\n",current_sb->getFSType() );

  current_sb->createInode ( sub_dentry, I_DIR );
  //debug ( VFSSYSCALL,"(mkdir) sub_dentry->getInode(): %d\n",sub_dentry->getInode() );
  return 0;
}


void VfsSyscall::readdir ( const char* pathname )
{
  //FileSystemInfo *fs_info = currentThread->getFSInfo();
  if ( dupChecking ( pathname ) == 0 )
  {
    path_walker_.pathRelease();
    char* path_tmp = new char[strlen ( fs_info->getName() ) + 1];
    strlcpy ( path_tmp, fs_info->getName(), ( strlen ( fs_info->getName() ) + 1 ) );
    fs_info->putName();

    char* char_tmp = strrchr ( path_tmp, SEPARATOR );
    assert ( char_tmp != 0 );

    // set directory
    uint32 path_prev_len = char_tmp - path_tmp + 1;
    fs_info->setName ( path_tmp, path_prev_len-1 );

    char* path_prev_name = fs_info->getName();

    int32 success = path_walker_.pathInit ( path_prev_name, 0 );
    if ( success == 0 )
    {
      success = path_walker_.pathWalk ( path_tmp );
    }
    fs_info->putName();

    if ( success != 0 )
    {
      //debug ( VFSSYSCALL,"(list) path_walker failed\n\n" );
      path_walker_.pathRelease();
      return;
    }

    Dentry* dentry = path_walker_.getDentry();

    if ( dentry->getInode()->getType() != I_DIR )
    {
      //debug ( VFSSYSCALL,"This path is not a directory\n\n" );
      return;
    }

    //debug ( VFSSYSCALL,"listing dir %s:\n",dentry->getName() );
    uint32 num_child = dentry->getNumChild();
    for ( uint32 i = 0; i<num_child; i++ )
    {
      Dentry *sub_dentry = dentry->getChild ( i );
      Inode* sub_inode = sub_dentry->getInode();
      uint32 inode_type = sub_inode->getType();
      switch ( inode_type )
      {
        case I_DIR:
          std::cout << "[D] ";
          break;
        case I_FILE:
          std::cout << "[F] ";
          break;
        case I_LNK:
        //  kprintf ( "[L] " );
          break;
        default:
          break;
      }
      std::cout << sub_dentry->getName() << std::endl;
    }
    delete[] path_tmp;
  }
  else
  {
    //debug ( VFSSYSCALL,"(list) Path doesn't exist\n" );
    path_walker_.pathRelease();
  }
}


int32 VfsSyscall::chdir ( const char* pathname )
{
  //FileSystemInfo *fs_info = currentThread->getFSInfo();
  if ( dupChecking ( pathname ) != 0 )
  {
    //kprintfd ( "Error: (chdir) the directory does not exist.\n" );
    path_walker_.pathRelease();
    fs_info->putName();
    return -1;
  }

  fs_info->putName();
  Dentry* current_dentry = path_walker_.getDentry();
  Inode* current_inode = current_dentry->getInode();
  if ( current_inode->getType() != I_DIR )
  {
    //debug ( VFSSYSCALL,"This path is not a directory\n\n" );
    path_walker_.pathRelease();
    return -1;
  }

  fs_info->setFsPwd ( path_walker_.getDentry() );//, path_walker.getVfsMount() );
  path_walker_.pathRelease();

  return 0;
}


int32 VfsSyscall::rm ( const char* pathname )
{
  //debug ( VFSSYSCALL,"(rm) name: %s\n",pathname );
  //FileSystemInfo *fs_info = currentThread->getFSInfo();
  if ( dupChecking ( pathname ) != 0 )
  {
    //kprintfd ( "Error: (rm) the directory does not exist.\n" );
    path_walker_.pathRelease();
    fs_info->putName();
    return -1;
  }
  //debug ( VFSSYSCALL,"(rm) \n" );
  fs_info->putName();
  Dentry* current_dentry = path_walker_.getDentry();
  //debug ( VFSSYSCALL,"(rm) current_dentry->getName(): %s \n",current_dentry->getName() );
  path_walker_.pathRelease();
  Inode* current_inode = current_dentry->getInode();
  //debug ( VFSSYSCALL,"(rm) current_inode: %d\n",current_inode );

  if ( current_inode->getType() != I_FILE )
  {
    //debug ( VFSSYSCALL,"This is not a file\n" );
    return -1;
  }

  Superblock* sb = current_inode->getSuperblock();
  if ( current_inode->rm() == INODE_DEAD )
  {
    //debug ( VFSSYSCALL,"remove the inode %d from the list of sb: %d\n",current_inode ,sb );
    sb->delete_inode ( current_inode );
    //debug ( VFSSYSCALL,"removed\n" );
  }
  else
  {
    //debug ( VFSSYSCALL,"remove the inode failed\n" );
    return -1;
  }

  return 0;
}


int32 VfsSyscall::rmdir ( const char* pathname )
{
  //FileSystemInfo *fs_info = currentThread->getFSInfo();
  if ( dupChecking ( pathname ) != 0 )
  {
    //kprintfd ( "Error: (rmdir) the directory does not exist.\n" );
    path_walker_.pathRelease();
    fs_info->putName();
    return -1;
  }

  fs_info->putName();
  Dentry* current_dentry = path_walker_.getDentry();
  path_walker_.pathRelease();
  Inode* current_inode = current_dentry->getInode();

  /*if ( current_dentry->getNumChild() != 0 )
  {
    //debug ( VFSSYSCALL,"This directory is not empty\n" );
    return -1;
  }*/

  if ( current_inode->getType() != I_DIR )
  {
    //debug ( VFSSYSCALL,"This is not a directory\n" );
    return -1;
  }

  Superblock* sb = current_inode->getSuperblock();
  if ( current_inode->rmdir() == INODE_DEAD )
  {
    //debug ( VFSSYSCALL,"remove the inode from the list\n" );
    sb->delete_inode ( current_inode );
  }
  else
  {
    //debug ( VFSSYSCALL,"remove the inode failed\n" );
    return -1;
  }

  return 0;
}


int32 VfsSyscall::close ( uint32 fd )
{
  File* file = 0;
  FileDescriptor* file_descriptor = 0;

  file_descriptor = getFileDescriptor ( fd );

  if ( file_descriptor == 0 )
  {
    //kprintfd ( "(close) Error: the fd does not exist.\n" );
    return -1;
  }

  file = file_descriptor->getFile();
  Inode* current_inode = file->getInode();
  Superblock *current_sb = current_inode->getSuperblock();
  current_sb->removeFd ( current_inode, file_descriptor );

  return 0;
}


int32 VfsSyscall::open ( const char* pathname, uint32 flag )
{
  //std::cout << "in open with pathname " << pathname << std::endl;
  //FileSystemInfo *fs_info = currentThread->getFSInfo();
  if ( flag > O_RDWR )
  {
    //debug ( VFSSYSCALL,"(open) invalid parameter flag\n" );
    return -1;
  }

  if ( dupChecking ( pathname ) == 0 )
  {
    //debug ( VFSSYSCALL,"(open) putName\n" );
    fs_info->putName();
    //debug ( VFSSYSCALL,"(open) path_walker.getDentry()\n" );
    Dentry* current_dentry = path_walker_.getDentry();
    //debug ( VFSSYSCALL,"(open) pathRelease\n" );
    path_walker_.pathRelease();
    //debug ( VFSSYSCALL,"(open)current_dentry->getInode() \n" );
    Inode* current_inode = current_dentry->getInode();
    //debug ( VFSSYSCALL,"(open) current_inode->getSuperblock()\n" );
    Superblock* current_sb = current_inode->getSuperblock();
    //debug ( VFSSYSCALL,"(open)getNumOpenedFile() \n" );
    uint32 num = current_inode->getNumOpenedFile();
    if ( num > 0 )
    {
      //debug ( VFSSYSCALL,"(open) repeated open\n" );
      // check the existing file
      //if ( flag != O_RDONLY )
      //{
        //kprintfd ( "(open) Error: The flag is not READ_ONLY\n" );
        //return -1;
      //}

      if ( current_inode->getType() != I_FILE )
      {
        //kprintfd ( "(open) Error: This path is not a file\n\n" );
        return -1;
      }

      File* file = current_inode->getFirstFile();
      uint32 file_flag = file->getFlag();
      if ( file_flag != O_RDONLY )
      {
        //kprintfd ( "(open) Error: The file flag is not READ_ONLY\n" );
        return -1;
      }
    }

    int32 fd = current_sb->createFd ( current_inode, flag );
    //std::cout << "the fd-num: " << fd << std::endl;

    return fd;
  }
  else
  {
    //debug ( VFSSYSCALL,"(open) create a new file\n" );
    path_walker_.pathRelease();
    char* path_tmp= new char[strlen ( fs_info->getName() ) + 1];
    strlcpy ( path_tmp, fs_info->getName(), ( strlen ( fs_info->getName() ) + 1 ) );
    fs_info->putName();

    char* char_tmp = strrchr ( path_tmp, SEPARATOR );
    assert ( char_tmp != 0 );

    // set directory
    uint32 path_prev_len = char_tmp - path_tmp + 1;
    fs_info->setName ( path_tmp, path_prev_len );

    char* path_prev_name = fs_info->getName();

    int32 success = path_walker_.pathInit ( path_prev_name, 0 );
    if ( success == 0 )
      success = path_walker_.pathWalk ( path_prev_name );

    if ( success != 0 )
    {
      //debug ( VFSSYSCALL,"(open) path_walker failed\n\n" );

      //std::cout << "couldn't walk to path, so now making dir" << std::endl;
      path_walker_.pathRelease();
      path_prev_name[path_prev_len-1] = 0;

      if(mkdir(path_prev_name, 0) == -1)
      {
        delete[] path_tmp;
        delete[] path_prev_name;
        return -1;
      }

      int32 success2 = path_walker_.pathInit ( path_prev_name, 0 );
      if ( success2 == 0 )
        success2 = path_walker_.pathWalk ( path_prev_name );
      if(success2 != 0)
      {
        path_walker_.pathRelease();
        delete[] path_tmp;
        delete[] path_prev_name;
        return -1;
      }
      //fs_info->putName();
      //return -1;
    }
    delete[] path_prev_name;

    Dentry* current_dentry = path_walker_.getDentry();
    path_walker_.pathRelease();
    Inode* current_inode = current_dentry->getInode();
    Superblock* current_sb = current_inode->getSuperblock();

    if ( current_inode->getType() != I_DIR )
    {
      //kprintfd ( "(open) Error: This path is not a directory\n\n" );
      delete[] path_tmp;
      return -1;
    }

    char_tmp++;
    uint32 path_next_len = strlen ( path_tmp ) - path_prev_len + 1;
    char* path_next_name = new char[path_next_len];
    strlcpy ( path_next_name, char_tmp, path_next_len );
    delete[] path_tmp;
    // create a new dentry
    Dentry *sub_dentry = new Dentry ( current_dentry );
    sub_dentry->setName ( path_next_name );
    delete[] path_next_name;
    sub_dentry->setParent ( current_dentry );
    //debug ( VFSSYSCALL,"(open) calling create Inode\n" );
    Inode* sub_inode = current_sb->createInode ( sub_dentry, I_FILE );
    //debug ( VFSSYSCALL,"(open) created Inode with dentry name %s\n", sub_inode->getDentry()->getName() );

    if ( !sub_inode )
    {
      delete sub_dentry;
      return -1;
    }

    int32 fd = current_sb->createFd ( sub_inode, flag );
    //debug ( VFSSYSCALL,"the fd-num: %d\n", fd );

    return fd;
  }
}


int32 VfsSyscall::read ( uint32 fd, char* buffer, uint32 count )
{
  FileDescriptor* file_descriptor = 0;

  file_descriptor = getFileDescriptor ( fd );

  if ( file_descriptor == 0 )
  {
    //kprintfd ( "(read) Error: the fd does not exist.\n" );
    return -1;
  }

  File* file = file_descriptor->getFile();
  return ( file->read ( buffer, count, 0 ) );
}


int32 VfsSyscall::write ( uint32 fd, const char *buffer, uint32 count )
{
  FileDescriptor* file_descriptor = 0;

  file_descriptor = getFileDescriptor ( fd );

  if ( file_descriptor == 0 )
  {
    //kprintfd ( "(read) Error: the fd does not exist.\n" );
    return -1;
  }

  File* file = file_descriptor->getFile();
  return ( file->write ( buffer, count, 0 ) );
}


int32 VfsSyscall::flush ( uint32 fd )
{
  FileDescriptor* file_descriptor = 0;

  file_descriptor = getFileDescriptor ( fd );

  if ( file_descriptor == 0 )
  {
    //kprintfd ( "(read) Error: the fd does not exist.\n" );
    return -1;
  }

  File* file = file_descriptor->getFile();

  return file->flush();
}


/*int32 VfsSyscall::mount ( const char *device_name, const char *dir_name, char *file_system_name, int32 flag )
{
  if ( strcmp ( file_system_name, "minixfs" ) == 0 )
  {
    MinixFSType *minixfs = new MinixFSType();
    vfs.registerFileSystem ( minixfs );
    return vfs.mount ( device_name, dir_name, file_system_name, flag );
  }
  return -1; // file system type not known
}*/


/*int32 VfsSyscall::umount ( const char *dir_name, int32 flag )
{
  return vfs.umount ( dir_name, flag );
}*/


uint32 VfsSyscall::getFileSize ( uint32 fd )
{
  FileDescriptor* file_descriptor = 0;

  file_descriptor = getFileDescriptor ( fd );

  if ( file_descriptor == 0 )
  {
    //kprintfd ( "(read) Error: the fd does not exist.\n" );
    return -1;
  }

  File* file = file_descriptor->getFile();
  return file->getSize();
}
