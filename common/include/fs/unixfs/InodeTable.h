/**
 * Filename: InodeTable.h
 * Description:
 *
 * Created on: 26.05.2012
 * Author: chris
 */

#ifndef INODETABLE_H_
#define INODETABLE_H_

#include "fs/FsDefinitions.h"
#include "fs/FsBitmap.h"

#include "cache/GeneralCache.h"

// forwards
class FileSystemUnix;
class FileSystemLock;
class Inode;
class FsVolumeManager;

/**
 * @class generic InodeTable interface for unix-styles file-systems
 * that use blocks or areas in the file-system to stored the I-Nodes
 * all together separated from the data
 * (example file-systems are minix, ext2,3,4, ...)
 */
class InodeTable : public Cache::DeviceAdapter
{
public:
  /**
   * InodeTable constructor
   * @param fs the associated Unix-style file-system
   * @param inode_table_sector_start the number of the first sector that holds
   * the InodeTable
   * @param num_sectors the number of sectors of the InodeTable so that
   * inode_table_sector_start + num_sectors addresses the last sector on the
   * Device belonging the InodeTable
   * @param inode_bitmap_start the start sector of the I-Node Bitmap
   * @param inode_bitmap_end the last sector of the I-Node Bitmap
   * @param inode_bitmap_num_bits the maximum number of I-Nodes on the FileSystem
   */
  InodeTable(FileSystemUnix* fs, FsVolumeManager* volume_manager,
             sector_addr_t inode_table_sector_start, sector_addr_t num_sectors,
             sector_addr_t inode_bitmap_start, sector_addr_t inode_bitmap_end,
             bitmap_t inode_bitmap_num_bits);

  /**
   * destructor
   */
  virtual ~InodeTable();

  /**
   * reads an I-Node from the FileSystem and returns an object instance
   * stored on the heap
   * @param id the ID of the I-Node to return
   * @return the requested I-Node
   */
  virtual Inode* getInode(inode_id_t id) = 0;

  /**
   * occupies and returns the id of the next free I-Node
   * @return the ID of the next free I-Node that is now occupied
   */
  virtual inode_id_t occupyAndReturnNextFreeInode(void) = 0;

  /**
   * storing I-Node data and information on the disk
   * @param id the ID of the I-Node to store
   * @param inode the I-Node data to store on the disk
   * @return true / false
   */
  virtual bool storeInode(inode_id_t id, Inode* inode) = 0;

  /**
   * removes a used i-node by freeing the Bitmap entry
   *
   * @param id the id of the i-node to remove
   * @return true in case of success / false in case of failure
   */
  virtual bool freeInode(inode_id_t id) = 0;

  /**
   * calculates the InodeTable Sector address of an I-Node with the
   * given ID
   *
   * @param id
   * @return the sector address of the InodeTable where the Inode is stored on
   */
  virtual sector_addr_t getInodeSectorAddr(inode_id_t id) = 0;

  /**
   * calculates the InodeTable Sector offset of an I-Node with the
   * given ID
   *
   * @param id
   * @return the sector offset of the InodeTable where the Inode is stored on
   */
  virtual sector_len_t getInodeSectorOffset(inode_id_t id) = 0;

  /**
   * [optional] inits a just created I-Node class instance
   *
   * @param inode
   * @param gid
   * @param uid
   * @param permissions
   */
  virtual void initInode(Inode* inode, uint32 gid = 0, uint32 uid = 0, uint32 permissions = 0755);

  /**
   * get the number of free I-Nodes
   *
   * @return number of usable I-Nodes
   */
  virtual bitmap_t getNumFreeInodes(void) const;

  /**
   * implements the DeviceAdapter read() method, this is just another
   * version of the getInode method!
   */
  virtual Cache::Item* read(const Cache::ItemIdentity& ident);

  /**
   * implements the DeviceAdapter's write() method
   */
  virtual bool write(const Cache::ItemIdentity& ident, Cache::Item* data);

  /**
   * removes the Item with the given Identity from the Device
   *
   * @param ident the Identity of the Item to remove
   * @param item the instance of the Item that should be deleted
   *
   * @return true if the Item was successfully removed, false if not
   */
  virtual bool remove(const Cache::ItemIdentity& ident, Cache::Item* item);

protected:

  // the associated file-system
  FileSystemUnix* fs_;

  // the Volume Manager of the Fs
  FsVolumeManager* volume_manager_;

  // the FS-Lock to synchronize the I-Node accesses
  //FileSystemLock lock_;

  // the start-sector of the I_Node Table
  sector_addr_t inode_tbl_start_;
  sector_addr_t num_sectors_;

  // the Inode-Bitmap
  FsBitmap inode_bitmap_;

private:
};

#endif /* INODETABLE_H_ */
