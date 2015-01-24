/**
 * @file RamFSSuperblock.h
 */

#ifndef RAM_FS_SUPERBLOCK_H__
#define RAM_FS_SUPERBLOCK_H__

#include "fs/Superblock.h"

class Inode;
class Superblock;

/**
 * @class RamFSSuperblock
 */
class RamFSSuperblock : public Superblock
{
  public:

    /**
     * constructor
     * @param s_root the root dentry of the new filesystem
     * @param s_dev the device number of the new filesystem
     */
    RamFSSuperblock ( Dentry* s_root, uint32 s_dev );

    /**
     * destructor
     */
    virtual ~RamFSSuperblock();

    /**
     * create a new Inode of the superblock, mknod with dentry, add in the list.
     * @param dentry the dentry to create the new inode with
     * @param type the inode type
     * @return the inode
     */
    virtual Inode* createInode ( Dentry* dentry, uint32 type );

    /**
     * remove the corresponding file descriptor.
     * @param inode the inode from which to remove the fd from
     * @param fd the fd to remove
     * @return 0 on success
     */
    virtual int32 removeFd ( Inode* inode, FileDescriptor* fd );

    /**
     * This method is called to read a specific inode from a mounted file-system.
     * @param inode the inode to read
     * @return 0 on success
     */
    virtual int32 readInode ( Inode* inode );

    /**
     * This method is called to write a specific inode to a mounted file-system,
     * and gets called on inodes which have been marked dirty.
     * @param inode the inode to write
     */
    virtual void writeInode ( Inode* inode );

    /**
     * This method is called whenever the reference count on an inode reaches 0,
     * and it is found that the link count (i_nlink= is also zero. It is
     * presumed that the file-system will deal with this situation be
     * invalidating the inode in the file-system and freeing up any resourses
     * used.
     * @param inode the inode to delete
     */
    virtual void delete_inode ( Inode* inode );

    /**
     * create a file with the given flag and a file descriptor with the given
     * inode.
     * @param inode the inode to create the fd for
     * @param flag the flag
     * @return the file descriptor
     */
    virtual int32 createFd ( Inode* inode, uint32 flag );
};
//-----------------------------------------------------------------------------

#endif // RamFSSuperblock_h___
