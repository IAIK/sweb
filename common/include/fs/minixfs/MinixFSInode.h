#ifndef MinixFSInode_h___
#define MinixFSInode_h___

#include "types.h"
#include "kstring.h"
#include "Inode.h"
#include "MinixFSZone.h"
#include <ulist.h>

/**
 * @class MinixFSInode represents an inode on a minix file system and handles the corresponding functions
 *
 */
class MinixFSInode : public Inode
{
    friend class MinixFSSuperblock;

  protected:

    /**
     * the zones storing the addresses of memory of this inode
     */
    MinixFSZone *i_zones_;

    /**
     * the inode number (the first inode has the i_num 1 on a minix file system)
     */
    uint32 i_num_;

    /**
     * reads all the inode's children from disc and creates their objects
     */
    virtual void loadChildren();

  public:

    /**
     * basic constructor
     * @param super_block the superblock the inode is on
     * @param inode_type the inode type (I_FILE, I_DIR)
     */
    MinixFSInode(Superblock *super_block, uint32 inode_type);

    /**
     * constructor of an inode existing on disc with all data given
     * @param super_block the superblock the inode is on
     * @param i_mode the mode containing the rights and the inode type (I_FILE, I_DIR)
     * @param i_size the inodes size
     * @param i_nlinks the number of links to this inode
     * @param i_zones the first 9 zone addresses
     * @param i_num the inode number
     */
    MinixFSInode(Superblock *super_block, uint16 i_mode, uint32 i_size, uint16 i_nlinks, uint32* i_zones, uint32 i_num);

    /**
     * destructor
     */
    virtual ~MinixFSInode();

    /**
     * lookup checks if that name (given by the char-array) exists in the
     * directory (I_DIR inode) and returns the Dentry if it does.
     * This involves finding and loading the inode. If the lookup failed to find
     * anything, this is indicated by returning NULL-pointer.
     * @param name the name to look for
     * @return the dentry found or NULL otherwise
     */
    virtual Dentry* lookup(const char *name);

    /**
     * The link method makes a hard link to the name referred to by the
     * denty, which is in the directory refered to by the Inode.
     * (only used for File)
     * @param flag the flag
     * @return the created file
     */
    virtual File* link(uint32 flag);

    /**
     * This removes the name refered to by the Dentry from the directory
     * referred to by the inode. (only used for File)
     * @param file the file to unlink
     * @return 0 on success
     */
    virtual int32 unlink(File* file);

    /**
     * creates a directory with the given dentry. It is only used to with directory.
     * @param dentry the dentry to create with
     * @return 0 on success
     */
    virtual int32 mkdir(Dentry *dentry);

    /**
     * removes the directory (if it is empty)
     * @return 0 on success
     */
    virtual int32 rmdir();

    /**
     * removes the named directory (if empty) or file (if not opened)
     * @return 0 on success
     */
    virtual int32 rm();

    /**
     * creates a directory with the given dentry.
     * @param dentry the dentry
     * @return 0 on success
     */
    virtual int32 mknod(Dentry *dentry); // no dir no file

    /**
     * creates a file with the given dentry.
     * @param dentry the dentry
     * @return 0 on success
     */
    virtual int32 mkfile(Dentry *dentry);

    /**
     * read the data from the inode
     * @param offset offset byte
     * @param size the size of data that read from this inode
     * @param buffer the dest char-array to store the data
     * @return the number of bytes read
     */
    virtual int32 readData(uint32 offset, uint32 size, char *buffer);

    /**
     * write the data to the inode
     * @param offset offset byte
     * @param size the size of data that write to this inode (data_)
     * @param buffer the src char-array
     * @return the number of bytes written
     */
    virtual int32 writeData(uint32 offset, uint32 size, const char *buffer);

    /**
     * flushes the inode to the file system
     * @return 0 on success
     */
    virtual int32 flush();

  private:
    /**
     * writes the inode dentry to disc
     * @param dest_i_num the inode number to write the dentry to
     * @param src_i_num the inode number to write
     * @param name the name to write there
     */
    void writeDentry(uint32 dest_i_num, uint32 src_i_num, const char* name);

    /**
     * finding the position of the dentry of the given inode in this inode
     * @param i_num the inode number to look for
     * @return the dentry position
     */
    int32 findDentry(uint32 i_num);

    /**
     * true if the inodes children are allready loaded
     */
    bool children_loaded_;

    ustl::list<Dentry*> other_dentries_;

};

#endif // Inode_h___
