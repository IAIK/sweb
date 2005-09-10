
#include "fs/PathWalker.h"

#include "fs/Inode.h"
#include "fs/Dentry.h"
#include "fs/fs_global.h"

#include "assert.h"
#include "util/string.h"

#include "mm/kmalloc.h"

#include "console/kprintf.h"

/// the pathWalker object
/// follow the inode of the corresponding file pathname
PathWalker path_walker;

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
  kprintfd("*** begin the init()\n");
  if(pathname == 0)
  {
    return PI_ENOTFOUND;
  }
  
  this->flags_ = flags;

  /// check the first character of the path
  if(*pathname == '/')
  {
    kprintfd("LAST_ROOT\n");
    this->last_type_ = LAST_ROOT;
    // altroot check
    
    this->dentry_ = fs_info.getRoot();
    this->vfs_mount_ = fs_info.getRootMnt();
  }
  else
  {
    kprintfd("Other\n");
    this->dentry_ = fs_info.getPwd();
    this->vfs_mount_ = fs_info.getPwdMnt();
  }
  
  kprintfd("*** end the init()\n");
  return PI_SUCCESS;
}

//----------------------------------------------------------------------
int32 PathWalker::pathWalk(const char* pathname)
{
  kprintfd("*** begin the pathWalk()\n");
  if(pathname == 0)
  {
    return PW_ENOTFOUND;
  }
  
  kprintfd("check the root\n");
  while(*pathname == '/')
    pathname++;
  if(!*pathname) // i.e. path = /
    return 0;

  kprintfd("pathWalk loop\n");
  bool parts_left = true;
  while(parts_left)
  {
    // get a part of pathname
    char* npart = 0;
    int32 npart_pos = getNextPart(pathname, npart);
    if(npart_pos < 0)
    {
      return PW_EINVALID;
    }
    
    if((*npart == '/0') || (npart_pos == 0))
    {
      delete npart;
      return PW_SUCCESS;
    }
    pathname += npart_pos;
    
    // checks the content
    this->last_ = npart;
    if(*npart == '.')
    {
      if(*(npart + 1) == '/0')
      {
        this->last_type_ = LAST_DOT;
      }
      else if((*(npart + 1) == '.') && (*(npart + 2) == '/0'))
      {
        this->last_type_ = LAST_DOTDOT;
      }
    }
    else
    {
      this->last_type_ = LAST_NORM;
    }

    // follow the inode
    if(this->last_type_ == LAST_DOT) // follow LAST_DOT
    {
      kfree(npart);
      last_ = 0;
      continue;
    }
    else if(this->last_type_ == LAST_DOTDOT) // follow LAST_DOTDOT
    {
      // case 1: the dentry_ is the root of file-system
      if((dentry_ == fs_info.getRoot()) && (vfs_mount_ = fs_info.getRootMnt()))
      {
        kfree(npart);
        last_ = 0;
        continue;
      }
      
      Dentry* parent_dentry = dentry_->getParent();
      // case 2: the parent_dentry is in the same file-system-type
      dentry_ = parent_dentry;
      kfree(npart);
      last_ = 0;
      continue;
      
      // case 3: the parent_dnetry isn't in the same file-system-type
      // update the vfs_mount_
    }
    else if(this->last_type_ == LAST_NORM) // follow LAST_NORM
    {
      Inode* current_inode = dentry_->getInode();
      Dentry *found = current_inode->lookup(last_);
      if(found != 0)
      {
        this->dentry_ = found;
      }
      else
      {
        return PW_ENOTFOUND;
      }
    }
    
    kfree(npart);
    last_ = 0;
    
    if(strlen(pathname) == 0)
    {
      parts_left = false;
    }
  }

  kprintfd("*** begin the pathWalker()\n");
  return 0;
}


//----------------------------------------------------------------------
int32 PathWalker::getNextPart(const char* path, char *npart)
{
  const char* tmp = 0;
  tmp = strchr(path, '/');

  char *npart_tmp = 0;
  int32 npart_len  = (size_t)(tmp - path);

  if(tmp == 0)
  {
    npart_len = strlen(path);
  }

  if(npart_len != 0)
  {
    npart_tmp = (char*)kmalloc(npart_len * sizeof(char)); /// + 1

    int32 copied = strlcpy(npart_tmp, path, npart_len);
    if (copied >= npart_len)
    {
      kfree(npart_tmp);
      return -1;
    }
  }

  npart = npart_tmp;
  return npart_len;
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

////----------------------------------------------------------------------
//char *PathWalker::skipSeparator(char const *path) const
//{
//  assert(path);
//
//  while (*path == '/')
//  {
//    ++path;
//  }
//
//  return path;
//}


