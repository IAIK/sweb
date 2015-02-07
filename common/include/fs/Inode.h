#ifndef INODE_H__
#define INODE_H__

#include "types.h"
#include "kprintf.h"
#include <ulist.h>

class Dentry;
class File;
class Superblock;

/**
 * three possible inode state bits:
 */
#define I_UNUSED 0 // the unused inode state
#define I_DIRTY 1 // Dirty inodes are on the per-super-block s_dirty_ list, and
// will be written next time a sync is requested.
#define I_LOCK 2  //state not implemented

/**
 * five possible inode type bits:
 */
#define I_FILE         0
#define I_DIR          1
#define I_LNK          2
#define I_CHARDEVICE   3
#define I_BLOCKDEVICE  4

/**
 * The per-inode flags:
 */
#define MS_NODEV 2 // If this inode is a device special file, it cannot be
// opend.
#define INODE_DEAD 666

/**
 * @class Inode
 * All information needed by the filesystem to handle a file is included in a
 * data class called an inode.
 */
class Inode
{
  protected:
    /**
     * The dentry of this inode. (dir)
     */
    Dentry *i_dentry_;

    /**
     * The dentry-List of this inode. (file)
     */
    ustl::list<Dentry*> i_dentry_link_;

    /**
     * The (open) file of this inode.
     */
    ustl::list<File*> i_files_;

    /**
     * the number of the link of this inode.
     */
    uint32 i_nlink_;

    /**
     * reference of superblock
     */
    Superblock *superblock_;

    /**
     * current file size in bytes
     */
    uint32 i_size_;

    /**
     * There are three possible inode type bits: I_FILE, I_DIR, I_LNK
     */
    uint32 i_type_;

    /**
     * There are three possible inode state bits: I_DIRTY, I_LOCK, I_UNUSED.
     */
    uint32 i_state_;

  public:

    /**
     * contructor
     * @param super_block the superblock to create the inode on
     * @param inode_type the inode type
     */
    Inode(Superblock *super_block, uint32 inode_type) :
        i_dentry_(0), i_nlink_(0), i_size_(0), i_state_(I_UNUSED)
    {
      superblock_ = super_block, i_type_ = inode_type;
    }

    /**
     * destructor
     */
    virtual ~Inode()
    {
    }

    /**
     * Create a directory with the given dentry.
     * @param dentry the dentry
     * @return 0 on success
     */
    virtual int32 create(Dentry *)
    {
      return 0;
    }

    /**
     * lookup should check if that name (given by the char-array) exists in the
     * directory (I_DIR inode) and should return the Dentry if it does.
     * This involves finding and loading the inode. If the lookup failed to find
     * anything, this is indicated by returning NULL-pointer.
     * @param name the name to look for
     * @return the dentry found
     */
    virtual Dentry* lookup(const char* /*name*/)
    {
      return 0;
    }

    /**
     * The link method should make a hard link to the name referred to by the
     * denty, which is in the directory refered to by the Inode.
     * (only used for File)
     * @param flag the flag
     * @return the link file
     */
    virtual File* link(uint32 /*flag*/)
    {
      return 0;
    }

    /**
     * This should remove the name refered to by the Dentry from the directory
     * referred to by the inode. (only used for File)
     * @param file the file to unlink
     * @retunr 0 on success
     */
    virtual int32 unlink(File* /*file*/)
    {
      return 0;
    }

    /**
     * This should create a symbolic link in the given directory with the given
     * name having the given value. It should d_instantiate the new inode into
     * the dentry on success.
     * @param inode the inode to link to
     * @param dentry yhe dentry to create the link in
     * @param link_name the name of the link to create
     * @return 0 on success
     */
    virtual int32 symlink(Inode */*inode*/, Dentry */*dentry*/, const char */*link_name*/)
    {
      return 0;
    }

    /**
     * Create a directory with the given dentry.
     * @param the dentry
     * @return 0 on success
     */
    virtual int32 mkdir(Dentry *)
    {
      return 0;
    }

    /**
     * Create a file with the given dentry.
     * @param dentry the dentry
     * @return 0 on success
     */
    virtual int32 mkfile(Dentry */*dentry*/)
    {
      return 0;
    }

    /**
     * Remove the named directory (if empty).
     * @return 0 on success
     */
    virtual int32 rmdir()
    {
      return 0;
    }

    /**
     * Remove the named directory (if empty) or file
     * @return 0 on success
     */
    virtual int32 rm()
    {
      return 0;
    }

    /**
     * Create a directory with the given dentry.
     * @param the dentry
     * @return 0 on success
     */
    virtual int32 mknod(Dentry *)
    {
      return 0;
    }

    /**
     * change the name to new_name
     * @param new name the new name
     * @retunr 0 on success
     */
    virtual int32 rename(const char* /*new_name*/)
    {
      return 0;
    }

    /**
     * The symbolic link referred to by the dentry is read and the value is
     * copied into the user buffer (with copy_to_user) with a maximum length
     * given by the integer.
     * @param dentry the dentry
     * @param max_length the maximum length
     * @return the number of bytes read
     */
    virtual int32 readlink(Dentry */*dentry*/, char*, int32 /*max_length*/)
    {
      return 0;
    }

    /**
     * If the directory (parent dentry) have a directory and a name within that
     * directory (child dentry) then the obvious result of following the name
     * from the directory would arrive at the child dentry. (for symlink)
     * @param prt_dentry the parent dentry
     * @param chd_dentry the child dentry
     * @return the dentry
     */
    virtual Dentry* followLink(Dentry */*prt_dentry*/, Dentry */*chd_dentry*/)
    {
      return 0;
    }

    /**
     * read the data from the inode
     * @param offset the offset from where to start
     * @param size the number of bytes to read
     * @param buffer where to store the read data
     * @return the number of bytes read
     */
    virtual int32 readData(uint32 /*offset*/, uint32 /*size*/, char */*buffer*/)
    {
      return 0;
    }

    /**
     * write the data to the inode
     * @param offset the offset from where to start writing
     * @param size the number of bytes to write
     * @param buffer where to write the data to
     * @return number of bytes written
     */
    virtual int32 writeData(uint32 /*offset*/, uint32 /*size*/, const char*/*buffer*/)
    {
      return 0;
    }

    /**
     * insert the opened file point to the file_list of this inode.
     * @param file the file to insert
     * @return 0 on success
     */
    int32 insertOpenedFiles(File*);

    /**
     * remove the opened file point from the file_list of this inode.
     * @param file the file to remove
     * @return 0 on success
     */
    int32 removeOpenedFiles(File*);

    /**
     * check the existance of the open-file-list
     * @return true if empty
     */
    bool openedFilesEmpty()
    {
      return (i_files_.empty());
    }

    /**
     * return the Superblock where this inode is located
     * @return the superblock
     */
    Superblock* getSuperblock()
    {
      return superblock_;
    }

    /**
     * setting the superblock is neccessary because the devices
     * are created before the DeviceFS is created
     * @param sb the superblock to set
     */
    void setSuperBlock(Superblock * sb)
    {
      superblock_ = sb;
    }

    /**
     * get the type from inode
     * @return the inodes type
     */
    uint32 getType()
    {
      return i_type_;
    }

    /**
     * get the pointer of the dentry
     * @return the inodes dentry
     */
    Dentry* getDentry()
    {
      return i_dentry_;
    }

    /**
     * get the first file object.
     * @return the first file
     */
    File* getFirstFile()
    {
      return i_files_.front();
    }

    /**
     * returns the number of opened files on this inode
     * @return the number of files
     */
    uint32 getNumOpenedFile()
    {
      return i_files_.size();
    }

    /**
     * returns the size
     * @return the size
     */
    uint32 getSize()
    {
      return i_size_;
    }

    /**
     * flushes the inode to the file system
     * @return 0 on success
     */
    int32 flush()
    {
      return 0;
    }
    ;

};

#endif // Inode_h___
