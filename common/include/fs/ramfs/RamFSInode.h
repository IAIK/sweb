/**
 * @file RamFSInode.h
 */

#ifndef RAM_FS_INODE_H__
#define RAM_FS_INODE_H__

#include "types.h"
#include "fs/Inode.h"

/**
 * @class RamFsInode
 */
class RamFSInode : public Inode
{
  protected:

    /**
     * the data of the inode
     */
    char* data_;

  public:



    /**
     * constructor
     * @param super_block the superblock to create the inode on
     * @param inode_type the inode type
     */
    RamFSInode ( Superblock *super_block, uint32 inode_type );

    /**
     * destructor
     */
    virtual ~RamFSInode();

    /**
     * Create a directory with the given dentry.
     * @param dentry the dentry
     * @return 0 on success
     */
    virtual int32 create ( Dentry *dentry );

    /**
     * lookup should check if that name (given by the char-array) exists in the
     * directory (I_DIR inode) and should return the Dentry if it does.
     * This involves finding and loading the inode. If the lookup failed to find
     * anything, this is indicated by returning NULL-pointer.
     * @param name the name
     * @return the found dentry
     */
    virtual Dentry* lookup ( const char *name );

    /**
     * The link method should make a hard link to the name referred to by the
     * denty, which is in the directory refered to by the Inode.
     * (only used for File)
     * @param flag the flag for the link
     * @return the link
     */
    virtual File* link ( uint32 flag );

    /**
     * This should remove the name refered to by the Dentry from the directory
     * referred to by the inode. (only used for File)
     * @param file the file to unlink
     * @return 0 on success
     */
    virtual int32 unlink ( File* file );

    /**
     * Create a directory with the given dentry. It is only used to with directory.
     * @param dentry the dentry
     * @return 0 on success
     */
    virtual int32 mkdir ( Dentry *dentry );

    /**
     * Remove the directory (if the sub_dentry is empty).
     * @return 0 on success
     */
    virtual int32 rmdir();

    /**
     * Removes the named directory (if empty) or file
     * @return 0 on success
     */
    virtual int32 rm();

    /**
     * Creates a directory with the given dentry.
     * @param dentry the dentry
     * @return 0 on success
     */
    virtual int32 mknod ( Dentry *dentry );

    /**
     * Creates a file with the given dentry.
     * @param dentry the dentry
     * @return 0 on success
     */
    virtual int32 mkfile ( Dentry *dentry );

    /// The symbolic link referred to by the dentry is read and the value is
    /// copied into the user buffer (with copy_to_user) with a maximum length
    /// given by the intege.
    virtual int32 readlink ( Dentry */*dentry*/, char*, int32 /*max_length*/ ) {return 0;}

    /// If the directory (parent dentry) have a directory and a name within that
    /// directory (child dentry) then the obvious result of following the name
    /// from the directory would arrive at the child dentry. If an inode requires
    /// some other, non-obvious, result - s do symbolic links - the inode should
    /// provide a follow_link method to return the appropriate new dentry.
    /// @prt_dentry the parent dentry
    /// @chd_dentry the child dentry
    /// @lookup_flags a number of LOOKUP flags
    virtual Dentry* followLink ( Dentry */*prt_dentry*/, Dentry */*chd_dentry*/ ) {return 0;}

    /// read the data from the inode
    /// @param offset offset byte
    /// @param size the size of data that read from this inode
    /// @buffer the dest char-array to store the data
    /// @return On successe, return 0. On error, return -1.
    virtual int32 readData ( uint32 offset, uint32 size, char *buffer );

    /// write the data to the inode
    /// @param offset offset byte
    /// @param size the size of data that write to this inode (data_)
    /// @buffer the src char-array
    /// @return On successe, return 0. On error, return -1.
    virtual int32 writeData ( uint32 offset, uint32 size, const char *buffer );

};

#endif // Inode_h___
