/**
 * Filename: UnixInodeCache.h
 * Description:
 *
 * Created on: 06.06.2012
 * Author: chris
 */

#ifndef UNIXINODECACHE_H_
#define UNIXINODECACHE_H_

#include "cache/CacheItem.h"

#include "fs/FsDefinitions.h"

class Inode;

class UnixInodeIdent : public Cache::ItemIdentity
{
public:
  UnixInodeIdent(inode_id_t inode_id);
  virtual ~UnixInodeIdent();

  /**
   * Comparison operator for two CacheItems
   */
  virtual bool operator==(const Cache::ItemIdentity& cmp);

  /**
   * clones this object and returns a new instance of it
   * Located on the heap
   * @return an identical copy of this identity
   */
  virtual Cache::ItemIdentity* clone(void) const;

  /**
   * getting the sector number
   */
  inode_id_t getInodeID(void) const;

private:

  // the sector-number is the ID
  inode_id_t inode_id_;
};

/**
 * @class the SectorCache Item
 */
class UnixInodeCacheItem : public Cache::Item
{
public:
  UnixInodeCacheItem(Inode* inode);
  virtual~ UnixInodeCacheItem();

  virtual void* getData(void);
  virtual const void* getData(void) const;

  Inode* getInode(void);
  const Inode* getInode(void) const;

private:

  // the Inode-data
  Inode* inode_;

};

#endif /* UNIXINODECACHE_H_ */
