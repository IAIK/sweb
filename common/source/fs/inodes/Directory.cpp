/**
 * Filename: Directory.cpp
 * Description:
 *
 * Created on: 13.05.2012
 * Author: chris
 */

#include "fs/inodes/Directory.h"
#include "fs/FileSystem.h"

#ifdef USE_FILE_SYSTEM_ON_GUEST_OS
#include "debug_print.h"
#define STL_NAMESPACE_PREFIX        std::
#else
#include "kprintf.h"
#define STL_NAMESPACE_PREFIX        ustl::
#endif

Directory::Directory(inode_id_t inode_number,
                     sector_addr_t device_sector, sector_len_t sector_offset,
                     FileSystem* file_system,
                     unix_time_stamp access_time,
                     unix_time_stamp mod_time, unix_time_stamp c_time,
                     uint32 ref_count, file_size_t size) :
    Inode(inode_number, device_sector, sector_offset, file_system,
          access_time, mod_time, c_time, ref_count, size),
    all_children_loaded_(false), mounted_fs_()
{
}

Directory::Directory(inode_id_t inode_number, FileSystem* file_system,
              unix_time_stamp access_time,
              unix_time_stamp mod_time,
              unix_time_stamp c_time) : Inode(inode_number, 0, 0, file_system,
                  access_time, mod_time, c_time),
    all_children_loaded_(false), mounted_fs_()
{
}

Directory::~Directory()
{
  debug(FS_INODE, "~Directory - CALL ID=%d\n", getID());
}

Inode::InodeType Directory::getType(void) const
{
  return InodeTypeDirectory;
}

uint32 Directory::getNumChildren(void) const
{
  return children_.size();
}

Inode* Directory::obtainInode(inode_id_t id, const char* name)
{
  debug(FS_INODE, "obtainingInode - request to load the i-node from the FS\n");

  Inode* inode = file_system_->acquireInode(id, this, name);

  if(inode != NULL && inode->getType() == Inode::InodeTypeDirectory)
  {
    // check if this directory is a mount point
    Directory* dir = static_cast<Directory*>(inode);

    if(dir->isMountPoint())
    {
      debug(FS_INODE, "obtainingInode - obtained inode is an active mount-point!\n");

      // free "real-directory" and return
      // instead the root-directory of the mounted file-system
      file_system_->releaseInode(inode);
      return dir->getMountedFs()->getRoot();
    }
  }

  return inode;
}

Inode* Directory::getInode(const char* filename)
{
  if(filename == NULL) return NULL;

  debug(FS_INODE, "getInode - getting child inode \"%s\"\n", filename);

  // finding an I-Node with that given filename
  for(ChildrenIterator it = children_.begin(); it != children_.end(); it++)
  {
    debug(FS_INODE, "getInode - child \"%s\"\n", (*it).first.c_str());

    if((*it).first == filename)
    {
      // I-Node does exists in the Directory, resolve the ID to the I-Node object
      return obtainInode(it->second, filename);
    }
  }

  return NULL;
}

Inode* Directory::getInode(uint32 index)
{
  // invalid index!
  if(index >= getNumChildren())
    return NULL;

  uint32 num = 0;
  for(ChildrenIterator it = children_.begin(); it != children_.end(); it++, num++)
  {
    debug(FS_INODE, "getInode - child \"%s\"\n", (*it).first.c_str());

    if(num == index)
    {
      debug(FS_INODE, "getInode - request to load the i-node from the FS\n");

      Inode* inode = obtainInode(it->second, it->first.c_str());

      // setting additional name informations for the readdir() call
      if(inode != NULL)
      {
        inode->setName(it->first.c_str());
      }
      return inode;
    }
  }

  // i-node not available
  return NULL;
}

Inode* Directory::getRealInode(const char* filename)
{
  if(filename == NULL) return NULL;

  debug(FS_INODE, "getInode - getting child inode \"%s\"\n", filename);

  // finding an I-Node with that given filename
  for(ChildrenIterator it = children_.begin(); it != children_.end(); it++)
  {
    debug(FS_INODE, "getInode - child \"%s\"\n", (*it).first.c_str());

    if((*it).first == filename)
    {
      // I-Node does exists in the Directory, resolve the ID to the I-Node object
      return file_system_->acquireInode(it->second, this, filename);
    }
  }

  return NULL;
}

bool Directory::isChild(const char* filename) const
{
  STL_NAMESPACE_PREFIX map<STL_NAMESPACE_PREFIX string, inode_id_t>::const_iterator it;

  // is there a child with the given name?
  for(it = children_.begin(); it != children_.end(); it++)
  {
    if((*it).first == filename)
    {
      return true;
    }
  }

  return false;
}

void Directory::addChild(const char* filename, inode_id_t inode_id)
{
  // check for double existence of such an I-Node
  if(!isChild(filename))
  {
    // I-Node with that name does not exist -> insert
    children_.insert(STL_NAMESPACE_PREFIX pair<STL_NAMESPACE_PREFIX string, inode_id_t>(filename, inode_id));
    debug(FS_INODE, "addChild - new inode \"%s\" added successful.\n", filename);
  }
  else
  {
    debug(FS_INODE, "addChild - ERROR child was already inserted.\n", filename);
  }
}

bool Directory::removeChild(const char* filename)
{
  children_.erase(filename);
  return true;
}

bool Directory::isEmpty(void) const
{
  debug(FS_INODE, "isEmpty - CALL (InodeID %d)\n", getID());

  // children are not loaded, so load them
  if(!areChildrenLoaded())
  {
    debug(FS_INODE, "isEmpty - ERROR children were not loaded!\n", getID());
    return false;
  }

  // 1. either the Directory has no children ...
  if(getNumChildren() == 0)
  {
    return true;
  }
  // 2. ... or the directory has ecatly 2 children and they are named "." and ".."
  else if(getNumChildren() == 2 && isChild(".") && isChild(".."))
  {
    return true;
  }

  // default: sorry directory is not empty
  return false;
}

void Directory::allChildrenLoaded(void)
{
  all_children_loaded_ = true;
}

void Directory::clearAllChildren(void)
{
  children_.clear();
  all_children_loaded_ = false;
}

bool Directory::areChildrenLoaded(void) const
{
  return all_children_loaded_;
}

void Directory::pushMountedFs(FileSystem* mounted_fs)
{
  mounted_fs_.push(mounted_fs);
}

bool Directory::popMountedFs(void)
{
  if(!mounted_fs_.empty())
  {
    mounted_fs_.pop();
    return true;
  }
  return false;
}

FileSystem* Directory::getMountedFs(void)
{
  if(mounted_fs_.empty())
    return NULL;

  return mounted_fs_.top();
}

bool Directory::isMountPoint(void) const
{
  return !mounted_fs_.empty();
}
