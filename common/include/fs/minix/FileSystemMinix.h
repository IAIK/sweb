/**
 * Filename: FileSystemMinix.h
 * Description:
 *
 * Created on: 16.05.2012
 * Author: chris
 */

#ifndef FILESYSTEMMINIX_H_
#define FILESYSTEMMINIX_H_

#include "fs/unixfs/FileSystemUnix.h"
#include "MinixDataStructs.h"

/**
 * @class FileSystemMinix - implementation of Minix Version 1 and 2
 * Version 3 differs a lot from the old versions, so its better to implement
 * Minix V3 in a different class (might be derived from here)
 */
class FileSystemMinix : public FileSystemUnix
{
public:
  /**
   * Minix File-System constructor
   *
   * @param device the Device hosting the Minix
   * @param mount_flags the mount flags specified
   * @param minix_version the Version of Minix to create
   * @param super_block the minix superblock of the FS
   * @param filename_len the used filename length might be 14 or 30!
   */
  FileSystemMinix(FsDevice* device, uint32 mount_flags,
                  minix_super_block super_block,
                  uint16 minix_version = 1, uint16 filename_len = 30);

  /**
   * destructor
   */
  virtual ~FileSystemMinix();

  /**
   * getting the partition Identifier Code of the File-System
   *
   * @return the Partition Identifier of Minix
   */
  virtual uint8 getPartitionIdentifier(void) const;

  /**
   * getting the name of the implemented FileSystem
   * @return the fs-name
   */
  virtual const char* getName(void) const;

  /**
   * creates a new file on the Device
   * @param parent the parent-dir of the new file
   * @param name the name of the new file
   * @param permissions the permissions of the File
   * @param uid the owner of the new File
   * @param gid the group-id of the new File
   * @return a new File instance or NULL in case or failure
   */
  virtual File* creat(Directory* parent, const char* name,
                      mode_t permissions, uid_t uid, gid_t gid);

  /**
   * creates a new empty directory on the FileSystem
   *
   * @param parent the parent-dir of the new Directory
   * @param name the name of the new Directory
   * @param cur_time the current timestamp before calling this method
   * @param permissions the Directorie's permissions
   * @param uid the directorie's owner
   * @param gid the owing group of the new Directory
   * @param[out] new_dir_id the ID of the just created Directory
   * @return 0 in case of success; < 0 in case of error
   */
  virtual int32 mkdir(Directory* parent, const char* name,
      unix_time_stamp cur_time,
      mode_t permissions, uid_t uid, gid_t gid,
      inode_id_t& new_dir_id);

  /**
   * removes an empty directory
   *
   * @param parent the parent Directory of the Directory to remove
   * @param id the ID of the Inode to remove
   * @param name the filename of sub-directory to remove
   * @return error code; 0 in case of success
   */
  virtual int32 rmdir(Directory* parent, inode_id_t inode_id, const char* name);

  /**
   * creates a hard link to an existing file
   * @param file the file to be hard-linked
   * @param dir the directory where the hard-link should be stored
   * @param name the new name for the existing file (hard-link's name)
   * @return TODO
   */
  virtual int32 link(File* file, Directory* parent, const char* name);

  /**
   * removes a hard link from a File
   * if this was the last hard-link to the file, the file will also be removed
   * @param file the file from which the hard link should be taken away
   * @param parent the parent directory of the hard-link
   * @param ref_name the name of the hard-link to be removed
   */
  virtual int32 unlink(File* file, Directory* parent, const char* ref_name);

  /**
   * returns a new instance of the statfs struct filled with current
   * Statistical data of the file-system
   *
   * @return
   */
  virtual statfs_s* statfs(void) const;

  /**
   * borrows and I-Node from the Cache for following operations (maybe read,
   * write or both). Do not forget to release the I-Node afterwards!
   * I-Node creation or deletion is considered as a Write-Operation
   * All Operations just dealing with the I-Node's data (also including data
   * changes, writes) are considered to be read-operations!
   *
   * @param id the ID of the I-Node to get (for FileSystem not using I-Node IDs set
   * pass value) for details refer to the concrete FS-implementation
   * @param parent the parent Directory holding the requested I-Node
   * @param name the (file)name of the requested I-Node (without the path!)
   * @return a pointer to the I-Node or NULL if an error happened
   */
  virtual Inode* acquireInode(inode_id_t id, Directory* parent, const char* name);

  /**
   * releases a borrowed I-Node
   *
   * @param id the borrowed I-Node that should be returned
   */
  virtual void releaseInode(Inode* inode);

  /**
   * adds a (re)-write operation, for the data stored in the I-Node object,
   * to the queue
   * @param inode the I-Node data to rewrite to the disk
   */
  virtual bool writeInode(Inode* inode);

  /**
   * destroys an Inode (but not any of the hard-link references to the inode)
   * frees its id, so that subsequently new created inodes can use it again
   * and marks all occupied data sectors as free
   * cleans the inode physically from the volume
   *
   * @param inode_ro_destroy the Inode to destroy
   * @return true if the Inode was successfully removed / false otherwise
   */
  virtual bool destroyInode(Inode* inode_ro_destroy);

  /**
   * getting the Block Size of the File System
   * @return the block size in bytes
   */
  virtual sector_len_t getBlockSize(void) const;

  /**
   * getting the size of a *data* block on the FileSystem
   * @return the *data* block size in bytes
   */
  virtual sector_len_t getDataBlockSize(void) const;

  /**
   * calculates the the sector address of the first sector of the given
   * data block
   *
   * @param data_block the the number (address) of the data block
   * @return the first sector address of the given data block address
   */
  virtual sector_addr_t convertDataBlockToSectorAddress(sector_addr_t data_block);

  /**
   * getting the first sector-address of the first data block
   *
   * @return the sector-address of the first data block
   */
  virtual sector_addr_t getFirstDataBlockAddress(void) const;

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
  virtual sector_addr_t occupyAndReturnFreeBlock(bool clear_block = true);

  /**
   * frees an occupied data-block of the FileSystem
   *
   * @param block_address the address of the data block that should be freed
   * @return true if the block was successfully freed; false in case the block
   * could not be freed (or it was already freed)
   */
  virtual bool freeOccupiedBlock(sector_addr_t block_address);

  /**
   * allocates a new free block of data on the device for the given
   * file and adds the number of the new sector to the I-Node's sector-list
   *
   * @param inode the I-node which data area should be extended
   * @param zero_out_sector if set to true all bytes of new sector will be
   * set to 0x00; if false all data will be untouched
   * @return the number of the new data-block in case of success; -1 if no more
   * block was available
   */
  virtual sector_addr_t appendSectorToInode(Inode* inode, bool zero_out_sector = false);

  /**
   * removes the n-th data block from the given I-Node (so that the sector can
   * be used again by other I-Node's). This causes the I-Node's size to shrink.
   * Nevertheless the file-size of the I-Node has to be decreased manually (it
   * is not done by this method!)
   *
   * @param inode the I-Node that should loose it's n-th data-block
   * @param sector_to_remove the number (n-th) sector to remove from the Inode
   * @return the address of the sector that was removed, or 0 in case of error
   */
  virtual sector_addr_t removeSectorFromInode(Inode* inode, uint32 sector_to_remove);

  /**
   * removes the last data block of the given I-Node (so that the sector can
   * be used again by other I-Node's). This causes the I-Node's size to shrink.
   * Nevertheless the file-size of the I-Node has to be decreased manually (it
   * is not done by this method!)
   *
   * @param inode
   * @return true in case of success / false
   */
  virtual sector_addr_t removeLastSectorOfInode(Inode* inode);

  /**
   * updates an I-Node's sector list (for details see FileSystem.h)
   *
   * @param inode the i-node thats list of blocks has to be updated
   */
  virtual void updateInodesSectorList(Inode* inode);

  /**
   * reads out the Super-Block from the FsDevice
   *
   * @param device
   * @param[out] superblock
   * @return true if the given Device hosts a valid Minix Superblock, false
   * if not or an error happened
   */
  static bool readSuperblock(FsDevice* device, minix_super_block& superblock);

protected:

  /**
   * (re)-loads a Directory's children to the memory form the Device
   * unsafe method : ONLY call from a WriteProtected context
   * @param parent the Directory to (re)load
   * @return true in case of success / false otherwise
   */
  virtual bool loadDirChildrenUnsafe(Directory* parent);

private:

  /**
   * initialise the root-inode
   */
  void initRootInode(void);

  /**
   * calculates the Bitmaps and Table offsets
   */
  void calculateOffsets(void);

  /**
   * inits and creates the InodeTable
   *
   * @param minix_version the Version of Minix to create
   */
  void createInodeTable(uint16 minix_version);

  /**
   * creates a new entry (hard link) for a child item in a Directory
   *
   * @param parent the parent Directory
   * @param name the name of the new hard-link entry
   * @param inode the id of the linked i-node
   */
  bool addDirectoryEntry(Directory* parent, const char* name, uint16 inode);

  /**
   * removes a hard link from a Directory
   *
   * @param parent
   * @param name
   * @param inode
   */
  bool removeDirectoryEntry(Directory* parent, const char* name, uint16 inode);

  /**
   * adds a new Dir-Entry to the given buffer
   *
   * @param buffer
   * @param buffer_offset
   * @param inode
   * @param name
   */
  void addDirEntryToBuffer(char* buffer, sector_len_t buffer_offset,
                           uint16 inode, const char* name);

  /**
   * checks whether the given data-block that belongs to a Directory
   * is completely free of references to child i-nodes
   *
   * @param buffer
   * @param buf_len
   *
   * @return true if no more references are on the data-block; false
   * if there is at least one remaining valid reference to a child
   */
  static bool isDataBlockFreeOfReferences(const char* buffer,
                            sector_len_t buf_len, uint16 directory_entry_len);

  // FS-Superblock
  minix_super_block superblock_;

  // Minix-Fs BlockSize and ZoneSize in bytes
  uint16 zone_size_;

  // the length of the filenames in the directory entries
  const uint16 FILENAME_LEN_;

  // the length of a directory-entry in bytes
  const uint16 DIR_ENTRY_SIZE_;

  // Zone-Bitmap
  FsBitmap* zone_bitmap_;

  // InodeTable offset
  sector_addr_t inode_bitmap_sector_;
  sector_addr_t inode_bitmap_size_;

  sector_addr_t zone_bitmap_sector_;
  sector_addr_t zone_bitmap_size_;

  sector_addr_t inode_tbl_sector_;
  sector_addr_t inode_tbl_size_;
};

#endif /* FILESYSTEMMINIX_H_ */
