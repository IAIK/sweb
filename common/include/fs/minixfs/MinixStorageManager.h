#ifndef MINIX_STORAGE_MANAGER__
#define MINIX_STORAGE_MANAGER__

#include "StorageManager.h"
#include "types.h"
#include "minix_fs_consts.h"

class MinixFSSuperblock;

/**
 * @class MinixStorageManager handles the inode and zone bitmaps of the minix file system
 * the minix file system's memory  is devided in zones,
 * a zone is a combination of 1 to many blocks depending on the size of the file system
 * there is a bitmap storing if a zone is in use by an inode or not
 * inode headers are stored after the bitmaps on the file system and contain mainly the inode number the zones of this inode and its size
 * there is a fixed number of inode headers which can be stored on a minix filesystem
 * the inode bitmap stores if a inode header is used or not
 */
class MinixStorageManager : public StorageManager
{
  public:

    /**
     * constructor
     * @param bm_buffer the buffer with the inode and zone bitmaps from disc
     * @param num_inode_bm_blocks the number of blocks used for the inode bitmap
     * @param num_zone_bm_blocks the number of blocks used for the zone bitmap
     * @param num_inodes the max number of inodes
     * @param num_zones the max number of zones
     */
    MinixStorageManager(char *bm_buffer, uint16 num_inode_bm_blocks, uint16 num_zone_bm_blocks, uint16 num_inodes,
                        uint16 num_zones);

    virtual ~MinixStorageManager();

    /**
     * returns the next free zone index and sets it as used
     * @return the zone index
     */
    virtual size_t allocZone();

    /**
     * returns the next free inode index and sets it as used
     * @return the inode index
     */
    virtual size_t allocInode();

    /**
     * unsets the zone at the given index
     * @param index the index
     */
    virtual void freeZone(size_t index);

    /**
     * unsets the inode at the given index
     * @param index the index
     */
    virtual void freeInode(size_t index);

    /**
     * checks if inode is set
     * @param index the inode index
     * @return true if the inode is set
     */
    virtual bool isInodeSet(size_t index);

    /**
     * get the number of inodes in use
     * @return the number of used inodes
     */
    virtual uint32 getNumUsedInodes();

    /**
     * writes the bitmaps to the minix file system
     * @param superblock the superblock of the minix file system to write to
     */
    void flush(MinixFSSuperblock *superblock);

    /**
     * prints the bitmaps to the command line
     */
    void printBitmap();

  private:

    size_t curr_zone_pos_;
    size_t curr_inode_pos_;

    uint32 num_inode_bm_blocks_;
    uint32 num_zone_bm_blocks_;

};

#endif //MINIX_STORAGE_MANAGER__

