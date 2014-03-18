/**
 * Filename: InodeTableMinix.h
 * Description:
 *
 * Created on: 06.06.2012
 * Author: chris
 */

#ifndef INODETABLEMINIX_H_
#define INODETABLEMINIX_H_

#include "fs/unixfs/InodeTable.h"

#include "fs/minix/MinixDataStructs.h"
#include "fs/inodes/Inode.h"

/**
 * @class InodeTableMinix
 */
class InodeTableMinix : public InodeTable
{
public:
  /**
   * InodeTableMinix constructor
   * @param fs
   * @param inode_table_sector_start the first sector of the I-Node table
   * @param num_sectors number of sectors occupied by the Inode-Table
   */
  InodeTableMinix(FileSystemUnix* fs, FsVolumeManager* volume_manager,
                  sector_addr_t inode_table_sector_start, sector_addr_t num_sectors,
                  sector_addr_t inode_bitmap_start, sector_addr_t inode_bitmap_end,
                  bitmap_t inode_bitmap_num_bits);

  /**
   * class destructor
   */
  virtual ~InodeTableMinix();

  /**
   * reads an I-Node from the FileSystem and returns an object instance
   * stored on the heap
   */
  virtual Inode* getInode(inode_id_t id);

  /**
   * occupies and returns the id of the next free I-Node
   * @return the ID of the next free I-Node that is now occupied
   */
  virtual inode_id_t occupyAndReturnNextFreeInode(void);

  /**
   * storing I-Node data and information on the disk
   * @param id the ID of the I-Node to store
   * @param inode the I-Node data to store on the disk
   * @return true / false
   */
  virtual bool storeInode(inode_id_t id, Inode* inode);

  /**
   * removes a used i-node by freeing the Bitmap entry
   *
   * @param id the id of the i-node to remove
   * @return true in case of success / false in case of failure
   */
  virtual bool freeInode(inode_id_t id);

  /**
   * calculates the InodeTable Sector address of an I-Node with the
   * given ID
   *
   * @param id
   * @return the sector address of the InodeTable where the Inode is stored on
   */
  virtual sector_addr_t getInodeSectorAddr(inode_id_t id);

  /**
   * calculates the InodeTable Sector offset of an I-Node with the
   * given ID
   *
   * @param id
   * @return the sector offset of the InodeTable where the Inode is stored on
   */
  virtual sector_len_t getInodeSectorOffset(inode_id_t id);

  /**
   * inits a just created I-Node class instance
   *
   * @param inode
   * @param gid
   * @param uid
   * @param permissions
   */
  void initInode(Inode* inode, uint32 gid = 0, uint32 uid = 0, uint32 permissions = 0775);

  /**
   * converts an InodeType value into a Minix - mode value
   * @param
   * @return
   */
  static uint16 convertInodeTypeToMinixMode(Inode::InodeType type);

private:

  /**
   * DBUG-PRINT: printing the contents/values of a minix_inode struct
   *
   * @param m_inode
   */
  static void printMinixInodeStruct(minix_inode* m_inode);

  /**
   * creates an I-Node object instance from the data read from
   * the Device
   * @param id
   * @param node_data
   * @return the I-node representation
   */
  Inode* createInodeFromDeviceData(inode_id_t id,
                                   sector_addr_t sector, sector_len_t offset,
                                   minix_inode* node_data);

  /**
   * Stores the data blocks of an i-node on the FileSystem's device (disk)
   * the in-memory linear list of the i-node has to be brought into the
   * minix format. 7-direct blocks + 1 single indirect + 1 double in-direct
   * block
   *
   * @param inode the i-node to store on the disk
   * @param node_data the disk-data structure to update
   */
  void storeDataBlocksToInode(Inode* inode, minix_inode* node_data);

  /**
   * the length of a minix data-block address in bytes
   */
  static const uint16 MINIX_DATA_BLOCK_ADDR_LEN = 2;
};

#endif /* INODETABLEMINIX_H_ */
