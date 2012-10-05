/**
 * Filename: InodeTable.cpp
 * Description:
 *
 * Created on: 26.05.2012
 * Author: chris
 */

#include "fs/unixfs/InodeTable.h"

#include "fs/FileSystemLock.h"
#include "fs/FileSystem.h"
#include "fs/unixfs/FileSystemUnix.h"
#include "fs/unixfs/UnixInodeCache.h"

#include "fs/inodes/Inode.h"

InodeTable::InodeTable(FileSystemUnix* fs, FsVolumeManager* volume_manager,
    sector_addr_t inode_table_sector_start, sector_addr_t num_sectors,
    sector_addr_t inode_bitmap_start, sector_addr_t inode_bitmap_end,
    bitmap_t inode_bitmap_num_bits) : fs_(fs), volume_manager_(volume_manager),
    inode_tbl_start_(inode_table_sector_start), num_sectors_(num_sectors),
    inode_bitmap_(fs, volume_manager,
                  inode_bitmap_start, inode_bitmap_end, inode_bitmap_num_bits)
{
}

InodeTable::~InodeTable()
{
}

Cache::Item* InodeTable::read(const Cache::ItemIdentity& ident)
{
  debug(INODE_TABLE, "read - CALL (ident addr=%x)\n", &ident);

  const UnixInodeIdent* unix_ident = reinterpret_cast<const UnixInodeIdent*>(&ident);
  //const UnixInodeIdent* unix_ident = dynamic_cast<const UnixInodeIdent*>(&ident);
  debug(INODE_TABLE, "Cache::read - going to read inode %d from Table\n", unix_ident->getInodeID());

  // invalid / not usable ident-object
  if(unix_ident == NULL)
  {
    debug(INODE_TABLE, "InodeTable::read - invalid parameter");
    return NULL;
  }

  // reading the I-Node's data from the FileSystem
  return new UnixInodeCacheItem( getInode( unix_ident->getInodeID() ) );
}

bool InodeTable::write(const Cache::ItemIdentity& ident, Cache::Item* data)
{
  debug(INODE_TABLE, "write - CALL (ident addr=%x) (item addr=%x)\n", &ident, data);

  const UnixInodeIdent* unix_ident = reinterpret_cast<const UnixInodeIdent*>(&ident);
  Inode* inode = reinterpret_cast<Inode*>(data->getData());

  if(unix_ident == NULL || inode == NULL)
  {
    debug(INODE_TABLE, "InodeTable::write - invalid parameter(s)");
    return false;
  }

  if(unix_ident->getInodeID() != inode->getID())
  {
    debug(INODE_TABLE, "InodeTable::write - Ident (&d) and Item (%d) ID mismatch.\n", unix_ident->getInodeID(), inode->getID());
    return false;
  }

  debug(INODE_TABLE, "InodeTable::write - Inode addr=%x ID=%d\n", inode, inode->getID());
  return storeInode(unix_ident->getInodeID(), inode);
}

bool InodeTable::remove(const Cache::ItemIdentity& ident, Cache::Item* item)
{
  debug(INODE_TABLE, "remove - CALL (ident addr=%x) (item addr=%x)\n", &ident, item);

  const UnixInodeIdent* unix_ident = reinterpret_cast<const UnixInodeIdent*>(&ident);
  UnixInodeCacheItem* unix_item = reinterpret_cast<UnixInodeCacheItem*>(item);

  if(unix_ident == NULL || unix_item == NULL)
  {
    debug(INODE_TABLE, "remove - invalid parameter(s)");
    return false;
  }

  // destroying the Inode physically by removing all Volume entries of the Inode
  if(!fs_->destroyInode(unix_item->getInode()))
  {
    debug(INODE_TABLE, "remove - failed to destroy the I-Node!");
    return false;
  }

  // finally free the Inode's ID entry in the InodeTable
  return freeInode( unix_ident->getInodeID() );
}

void InodeTable::initInode(Inode* inode __attribute__((unused)),
                           uint32 gid __attribute__((unused)),
                           uint32 uid __attribute__((unused)),
                           uint32 permissions __attribute__((unused)))
{
  // does nothing by default
}

bitmap_t InodeTable::getNumFreeInodes(void) const
{
  return inode_bitmap_.getNumFreeBits();
}
