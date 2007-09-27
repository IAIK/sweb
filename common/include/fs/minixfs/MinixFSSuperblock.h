/**
 * @file MinixFSSuperblock.h
 */

#ifndef MinixFSSuperblock_h___
#define MinixFSSuperblock_h___

#include "fs/PointList.h"
#include "fs/Superblock.h"
#include "MinixStorageManager.h"

class Inode;
class MinixFSInode;
class Superblock;

/**
 * @class MinixFSSuperblock stores the data of the superblock on a minix file system.
 * handles its inodes and memory
 * knows how to read and write data from and to the file system
 */
class MinixFSSuperblock : public Superblock
{
  public:
    friend class MinixFSInode;
    friend class MinixFSZone;
    friend class MinixStorageManager;

    /**
     * constructor
     * @param s_root
     * @param s_dev
     * @return
     */
    MinixFSSuperblock ( Dentry* s_root, uint32 s_dev );

    /**
     * destructor
     * @return
     */
    virtual ~MinixFSSuperblock();

    /**
     * creates one new inode of the superblock adds the dentry sets it in the bitmap
     * @param dentry the dentry to set in the new inode
     * @param type the file type of the new inode (I_DIR, I_FILE)
     * @return the new inode
     */
    virtual Inode* createInode ( Dentry* dentry, uint32 type );

    /**
     *  remove the corresponding file descriptor and unlinks it from the inode.
     * @param inode the inode the file descriptor is linked to
     * @param fd the file descriptor
     * @return 0 on success
     */
    virtual int32 removeFd ( Inode* inode, FileDescriptor* fd );

    /**
     * reads one inode from the mounted file system
     * @param inode the inode to read
     * @return 0 on success
     */
    virtual int32 readInode ( Inode* inode );

    /**
     * writes the inode from the mounted file system
     * @param inode the inode to write
     */
    virtual void writeInode ( Inode* inode );

    /**
     * removes one inode from the file system and frees all its resources
     * @param inode the inode to delete
     */
    virtual void delete_inode ( Inode* inode );

    /**
     * create a file with the given flag and a file descriptor with the given inode.
     * @param inode the inode to link the file with
     * @param flag the flag to create the file with
     * @return the file descriptor
     */
    virtual int32 createFd ( Inode* inode, uint32 flag );

    /**
     * allocates one zone on the file system
     * @return the zone index
     */
    virtual uint16 allocateZone();


  protected:

    /**
     * creates an Inode object with the given number from the file system
     * @param i_num the inode number
     * @return the Inode object
     */
    MinixFSInode *getInode ( uint16 i_num );

    /**
     * reads one Zone from the file system to the given buffer
     * @param zone the zone index to read
     * @param buffer the buffer to write in
     */
    void readZone ( uint16 zone, Buffer *buffer );

    /**
     * reads the given number of blocks from the file system to the given buffer
     * @param block the index of the block to start reading
     * @param num_blocks the number of blcoks to read
     * @param buffer the buffer to write in
     */
    void readBlocks ( uint16 block, uint32 num_blocks, Buffer *buffer );

    /**
     * writes one zone from the given buffer to the file system
     * @param zone the zone index to write
     * @param buffer the buffer to write
     */
    void writeZone ( uint16 zone, Buffer *buffer );

    /**
     * writes the given number of blcoks to the file system from the given buffer
     * @param block the index of the first block to write
     * @param num_blocks the number of blocks to write
     * @param buffer the buffer to write
     */
    void writeBlocks ( uint16 block, uint32 num_blocks, Buffer *buffer );

    /**
     * writes the given number of bytes to the filesystem
     * the bytes must be on one block
     * @param block the block to write to
     * @param offset the offset on the block
     * @param size the number of bytes to write
     * @param buffer the buffer with the bytes to write
     * @return the number of bytes written
     */
    int32 writeBytes ( uint32 block, uint32 offset, uint32 size, Buffer *buffer );

    /**
     * reads the given number of bytes from the disc
     * the bytes must be on one block
     * @param block the block to read from
     * @param offset the offset on the block
     * @param size the number of bytes to read
     * @param buffer the buffer to write to
     * @return the number of bytes read
     */
    int32 readBytes ( uint32 block, uint32 offset, uint32 size, Buffer *buffer );

  private:

    /**
     * reads the root inode and its children from the filesystem
     */
    void initInodes();

    /**
     * # usable inodes on the minor device
     */
    uint16 s_num_inodes_;
    /**
     * total device size including bit maps etc.
     */
    uint16 s_num_zones_;
    /**
     * # of blocks used by inode bit map
     */
    uint16 s_num_inode_bm_blocks_;
    /**
     * # of blocks used by zone bit map
     */
    uint16 s_num_zone_bm_blocks_;
    /**
     * number of first datazone
     */
    uint16 s_1st_datazone_;
    /**
     * log2 of blocks/zone
     */
    uint16 s_log_zone_size_;
    /**
     * maximum file size on this device
     */
    uint32 s_max_file_size_;
    /**
     * the storage manager
     */
    MinixStorageManager* storage_manager_;
};

#endif // MinixFSSuperblock_h___
