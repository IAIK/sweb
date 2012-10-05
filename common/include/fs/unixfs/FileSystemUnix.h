/**
 * Filename: FileSystemUnix.h
 * Description:
 *
 * Created on: 19.07.2012
 * Author: chris
 */

#ifndef FILESYSTEMUNIX_H_
#define FILESYSTEMUNIX_H_

#include "FileSystem.h"

// forwards
class InodeTable;

// number of an unused data-block (address)
#define UNUSED_DATA_BLOCK           0x00

/**
 * @class a generalization class for a unix-style file-systems
 */
class FileSystemUnix : public FileSystem
{
  friend class InodeTable;
public:
  /**
   * constructor
   * @param device
   */
  FileSystemUnix(FsDevice* device, uint32 mount_flags = 0x00);

  /**
   * destructor
   */
  virtual ~FileSystemUnix();

  /**
   * synchronizes all changes to the Inode
   *
   * @param inode the I-Node that should be synched with the FileSystem
   * @return 0 is case of success; a value < 0 in case of failure
   */
  virtual int32 fsync(Inode* inode);

  /**
   * occupies and returns a free data block of the device
   *
   * @param clear_block if this flag is set to true all bytes of
   * the data block will be set to 0x00; if the flag is false
   * nothing will be done
   *
   * @return the sector address of the occupied sector or 0 in case
   * the device is full
   */
  virtual sector_addr_t occupyAndReturnFreeBlock(bool clear_block = true) = 0;

  /**
   * frees an occupied data-block of the FileSystem
   *
   * @param block_address the address of the data block that should be freed
   * @return true if the block was successfully freed; false in case the block
   * could not be freed (or it was already freed)
   */
  virtual bool freeOccupiedBlock(sector_addr_t block_address) = 0;

  /**
   * resolves an indirect data block and inserts the single (direct) data blocks
   * in the i-nodes list
   *
   * NOTE: the caller has to assert that the given indirect_block really contains
   * valid indirect sector addresses and that the degree_of_indirection is
   * exactly correct! The given values can not be validated!
   *
   * @param inode the i-node where the new direct data blocks are added to
   * @param indirect_block
   * @param degree_of_indirection
   * @param sector_addr_len the FileSystem's specific length of sector addresses
   * in bytes(!)
   */
  static void resolveIndirectDataBlocks(Inode* inode, sector_addr_t indirect_block,
      uint16 degree_of_indirection, uint16 sector_addr_len);

  /**
   * storeIndirectDataBlocks - stores the Inode's linear list of data-blocks
   * into the arrangement of n-indirect addressed blocks
   *
   * NOTE: if the Inode's size decreased the remaining unused blocks of
   * indirect addressing will be freed!
   *
   * @param inode the Inode which's sectors should be stored to volume
   * @param inode_sector_to_store the number of the Inode's first data-block
   * that should be stored to volume
   * @param indirect_block the address of the first indirect data-block to
   * start with
   * @param degree_of_indirection the degree of indirection min value is 1!
   * @param sector_addr_len the length of disk addresses of the FileSystem
   * in bytes!
   *
   * @return the address of the first sector in the sequence of indirect
   * addressed sectors or -1U in case of error
   * e.g. the function was called with invalid arguments, an I/O write
   * error happened and so on.
   */
  static sector_addr_t storeIndirectDataBlocks(Inode* inode, sector_addr_t inode_sector_to_store,
                                      sector_addr_t indirect_block, uint16 degree_of_indirection,
                                      uint16 sector_addr_len);

protected:

  /**
   * makes and returns a safe escaped string from a given fixed length string
   * that might not be escaped safely
   *
   * @param filename the fixed length that might not be escaped well
   * @param max_filename_len the length of the given string
   *
   * @return the safe escaped string
   */
  char* safeEscapeFilename(const char* filename, uint32 filename_len);

  // Unix-style FileSystems all have at least one InodeTable
  InodeTable* inode_table_;
};

#endif /* FILESYSTEMUNIX_H_ */
