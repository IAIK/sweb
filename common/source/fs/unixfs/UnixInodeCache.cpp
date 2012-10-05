/**
 * Filename: UnixInodeCache.cpp
 * Description:
 *
 * Created on: 06.06.2012
 * Author: chris
 */

#include "fs/unixfs/UnixInodeCache.h"
#include "fs/inodes/Inode.h"

#ifdef USE_FILE_SYSTEM_ON_GUEST_OS
#include <cstring>
#endif

UnixInodeIdent::UnixInodeIdent(inode_id_t inode_id) : inode_id_(inode_id)
{
}

UnixInodeIdent::~UnixInodeIdent()
{
}

bool UnixInodeIdent::operator==(const Cache::ItemIdentity& cmp)
{
  // try to cast the Interface into a SectorIdent type, if it fails
  // we know that they are not equal!
  const UnixInodeIdent* p_cmp = reinterpret_cast<const UnixInodeIdent*>(&cmp);

  //const SectorCacheIdent* p_cmp = dynamic_cast<const SectorCacheIdent*>(&cmp);

  if(p_cmp == NULL)
  {
    // the given Ident object is not the same type as this
    return false;
  }

  return (p_cmp->getInodeID() == this->getInodeID());
}

Cache::ItemIdentity* UnixInodeIdent::clone(void) const
{
  return new UnixInodeIdent(getInodeID());
}

inode_id_t UnixInodeIdent::getInodeID(void) const
{
  return inode_id_;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

UnixInodeCacheItem::UnixInodeCacheItem(Inode* inode) : Item(),
    inode_(inode)
{
}

UnixInodeCacheItem::~UnixInodeCacheItem()
{
  if(inode_ != NULL)
    delete inode_;
}

void* UnixInodeCacheItem::getData(void)
{
  return inode_;
}

const void* UnixInodeCacheItem::getData(void) const
{
  return inode_;
}

Inode* UnixInodeCacheItem::getInode(void)
{
  return inode_;
}

const Inode* UnixInodeCacheItem::getInode(void) const
{
  return inode_;
}
