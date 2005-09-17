
#include "fs/PathWalker.h"

#include "fs/Inode.h"
#include "fs/Dentry.h"
#include "fs/VfsMount.h"
#include "fs/fs_global.h"
#include "fs/Superblock.h"

#include "assert.h"
#include "util/string.h"

#include "mm/kmalloc.h"
#include "console/kprintf.h"

/// the pathWalker object
/// follow the inode of the corresponding file pathname
PathWalker path_walker;

#define CHAR_DOT '.'
#define NULL_CHAR '\0'
#define CHAR_ROOT '/'
#define SEPARATOR '/'

//----------------------------------------------------------------------
PathWalker::PathWalker()
{
}

//----------------------------------------------------------------------
PathWalker::~PathWalker()
{
}

//----------------------------------------------------------------------
int32 PathWalker::pathInit(const char* pathname, uint32 flags)
{
  if(pathname == 0)
  {
    return PI_ENOTFOUND;
  }

  this->flags_ = flags;

  /// check the first character of the path
  if(*pathname == CHAR_ROOT)
  {
    this->last_type_ = LAST_ROOT;
    // altroot check

    // start with ROOT
    this->dentry_ = fs_info.getRoot();
    this->vfs_mount_ = fs_info.getRootMnt();
  }
  else
  {
    // start with PWD
    this->dentry_ = fs_info.getPwd();
    this->vfs_mount_ = fs_info.getPwdMnt();
  }
  
  if((dentry_ == 0) || (vfs_mount_ == 0))
    return PI_ENOTFOUND;

  return PI_SUCCESS;
}

//----------------------------------------------------------------------
int32 PathWalker::pathWalk(const char* pathname)
{
  if(pathname == 0)
  {
    return PW_ENOTFOUND;
  }

  while(*pathname == SEPARATOR)
    pathname++;
  if(!*pathname) // i.e. path = /
    return 0;

  bool parts_left = true;
  while(parts_left)
  {
    char* npart = 0;
    int32 npart_pos = 0;
    npart = getNextPart(pathname, npart_pos);
    if(npart_pos < 0)
    {
      return PW_EINVALID;
    }

    if((*npart == NULL_CHAR) || (npart_pos == 0))
    {
      delete npart;
      return PW_SUCCESS;
    }
    pathname += npart_pos;

    this->last_ = npart;
    if(*npart == CHAR_DOT)
    {
      if(*(npart + 1) == NULL_CHAR)
      {
        this->last_type_ = LAST_DOT;
      }
      else if((*(npart + 1) == CHAR_DOT) && (*(npart + 2) == NULL_CHAR))
      {
        this->last_type_ = LAST_DOTDOT;
      }
    }
    else
    {
      this->last_type_ = LAST_NORM;
    }

    // follow the inode
    // check the VfsMount
    if(this->last_type_ == LAST_DOT) // follow LAST_DOT
    {
      kfree(npart);
      last_ = 0;
      continue;
    }
    else if(this->last_type_ == LAST_DOTDOT) // follow LAST_DOTDOT
    {
      kfree(npart);
      last_ = 0;

      if((dentry_ == fs_info.getRoot())&&(vfs_mount_ == fs_info.getRootMnt()))
      {
        // the dentry_ is the root of file-system
        // because the ROOT has not parent from VfsMount.
        continue;
      }

      VfsMount* vfs_mount = vfs.getVfsMount(dentry_, true);
      if(vfs_mount != 0)
      {
        // the dentry_ is a mount-point
        vfs_mount_ = vfs_mount->getParent();
        dentry_ = vfs_mount->getMountPoint();
      }
      Dentry* parent_dentry = dentry_->getParent();
      dentry_ = parent_dentry;
      continue;
    }
    else if(this->last_type_ == LAST_NORM) // follow LAST_NORM
    {
      Inode* current_inode = dentry_->getInode();
      Dentry *found = current_inode->lookup(last_);
      
      kfree(npart);
      last_ = 0;
      if(found != 0)
      {
        this->dentry_ = found;
      }
      else
      {
        return PW_ENOTFOUND;
      }

      VfsMount* vfs_mount = vfs.getVfsMount(dentry_);

      if(vfs_mount != 0)
      {
        kprintfd("MOUNT_DOWN\n");
        // the dentry_ is a mount-point
        // update the vfs_mount_
        vfs_mount_ = vfs_mount;
        
        // change the dentry of the mount-point
        dentry_ = vfs_mount_->getRoot();

      }
    }

    while(*pathname == SEPARATOR)
      pathname++;

    if(strlen(pathname) == 0)
    {
      break;
    }
  }

  return 0;
}

//----------------------------------------------------------------------
char* PathWalker::getNextPart(const char* path, int32 &npart_len)
{
  char* tmp = 0;
  tmp = strchr(path, SEPARATOR);

  char* npart = 0;
  npart_len = (size_t)(tmp - path + 1);

  uint32 length = npart_len;

  if(tmp == 0)
  {
    npart_len = strlen(path);
    length = npart_len + 1;
  }

  if(length != 0)
  {
    npart = (char*)kmalloc(length * sizeof(char));
    strlcpy(npart, path, length);
  }

  return npart;
}
//----------------------------------------------------------------------
void PathWalker::pathRelease()
{
  dentry_ = 0;
  vfs_mount_ = 0;
  flags_ = 0;
  last_type_ = 0;
  last_ = 0;
}

//----------------------------------------------------------------------
char *PathWalker::skipSeparator(char const */*path*/) const
{
//  assert(path);
//
//  while (*path == '/')
//  {
//    ++path;
//  }
//
//  return path;
}


