
#include "fs/PathWalker.h"

#include "fs/Inode.h"
#include "fs/Dentry.h"
#include "fs/fs_global.h"

#include "assert.h"
#include "util/string.h"

#include "mm/kmalloc.h"

//----------------------------------------------------------------------
PathWalker::PathWalker()
{
}

//----------------------------------------------------------------------
PathWalker::~PathWalker()
{
}

//----------------------------------------------------------------------
int32 PathWalker::init(const char* name, uint32 flags)
{
  assert(name);

  flags_ = flags;

  /// TODO get PWD and Mount from current process

  const char* pos = name;

  if (*pos++ == '/')
  {
    this->last_type_ = LAST_ROOT;
    // TODO set last_ to the root directory
    // and set dentry_- to be the root directory.
  }
  while (*pos++ == '/');

  bool parts_left = true;
  while (parts_left)
  {
    char *npart = 0;
    int32 npart_pos = getNextPart(pos, npart);
    if (npart_pos < 0)
    {
      return PW_EINVALID;
    }

    if (*npart == '\0')
    {
      delete npart;
      return PW_SUCCESS;
    }

    this->last_ = npart;
    if (*npart == '.')
    {
      if (*(npart + 1) ==  '\0')
      {
        this->last_type_ = LAST_DOT;
      }
      else if (*(npart + 1) ==  '.')
      {
        this->last_type_ = LAST_DOTDOT;
      }
    }
    else
    {
      this->last_type_ = LAST_NORM;
    }

    Inode *current_inode = dentry_->get_inode();
    // TODO check the permisiions

    if (this->last_type_ == LAST_NORM)
    {
      Dentry to_find(this->dentry_);
      Dentry *found = 0;
      to_find.set_name(last_, strlen(last_));
      if ((found = current_inode->lookup(&to_find)))
      {
        this->dentry_ =  found;
      }
      else
      {
        return PW_ENOTFOUND;
      }
    }

  }

  return 0;
}


//----------------------------------------------------------------------
int32 PathWalker::getNextPart(const char* path, char *npart)
{
  const char* tmp = 0;

  if ((tmp = strchr(path, '/')))
  {
    int32 npart_len  = (size_t)(tmp -path);
    char *npart_tmp = (char*)kmalloc(npart_len * sizeof(char));

    int32 copied = strlcpy(npart_tmp, path, npart_len);
    if (copied >= npart_len)
    {
      kfree(npart_tmp);
      return -1;
    }

    npart = npart_tmp;
    return npart_len;
  }

  return 0;
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


