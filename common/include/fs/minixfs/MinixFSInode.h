// Projectname: SWEB
// Simple operating system for educational purposes

#ifndef MinixFSInode_h___
#define MinixFSInode_h___

#include "types.h"
#include "fs/PointList.h"
#include "fs/Inode.h"

//-------------------------------------------------------------------------
/**
 * MinixFSInode
 *
 */
class MinixFSInode : public Inode
{
  friend class MinixFSSuperblock;
  protected:

    uint16* i_zones_;

  public:

  /// constructor
    MinixFSInode(Superblock *super_block, uint32 inode_type);

    MinixFSInode(Superblock *super_block, uint16 i_mode, uint16 i_uid, uint32 i_size, uint32 i_modtime, uint8 i_gid, uint8 i_nlinks, uint16* i_zones);
    
  /// destructor
    virtual ~MinixFSInode();

  /// Create a directory with the given dentry.
    virtual int32 create(Dentry *dentry);

  /// lookup should check if that name (given by the char-array) exists in the
  /// directory (I_DIR inode) and should return the Dentry if it does.
  /// This involves finding and loading the inode. If the lookup failed to find
  /// anything, this is indicated by returning NULL-pointer.
    virtual Dentry* lookup(const char *name);

  /// The link method should make a hard link to the name referred to by the
  /// denty, which is in the directory refered to by the Inode.
  /// (only used for File)
    virtual File* link(uint32 flag);

  /// This should remove the name refered to by the Dentry from the directory
  /// referred to by the inode. (only used for File)
    virtual int32 unlink(File* file);

  /// This should create a symbolic link in the given directory with the given
  /// name having the given value. It should d_instantiate the new inode into
  /// the dentry on success.
    virtual int32 symlink(Inode */*inode*/, Dentry */*dentry*/,
                          const char */*link_name*/) {return 0;}

  /// Create a directory with the given dentry. It is only used to with directory.
    virtual int32 mkdir(Dentry *dentry);

  /// Remove the directory (if the sub_dentry is empty).
    virtual int32 rmdir();

  /// REmove the named directory (if empty) or file
    virtual int32 rm();

  /// Create a directory with the given dentry.
    virtual int32 mknod(Dentry *dentry);

  /// Create a file with the given dentry.
    virtual int32 mkfile(Dentry *dentry);

  /// change the name to new_name
    virtual int32 rename(const char* /*new_name*/) {return 0;}

  /// The symbolic link referred to by the dentry is read and the value is
  /// copied into the user buffer (with copy_to_user) with a maximum length
  /// given by the intege.
    virtual int32 readlink(Dentry */*dentry*/, char*, int32 /*max_length*/) {return 0;}

  /// If the directory (parent dentry) have a directory and a name within that
  /// directory (child dentry) then the obvious result of following the name
  /// from the directory would arrive at the child dentry. If an inode requires
  /// some other, non-obvious, result - s do symbolic links - the inode should
  /// provide a follow_link method to return the appropriate new dentry.
  /// @prt_dentry the parent dentry
  /// @chd_dentry the child dentry
  /// @lookup_flags a number of LOOKUP flags
    virtual Dentry* followLink(Dentry */*prt_dentry*/, Dentry */*chd_dentry*/) {return 0;}

  /// read the data from the inode
  /// @param offset offset byte
  /// @param size the size of data that read from this inode
  /// @buffer the dest char-array to store the data
  /// @return On successe, return 0. On error, return -1.
    virtual int32 readData(uint32 offset, uint32 size, char *buffer);

  /// write the data to the inode
  /// @param offset offset byte
  /// @param size the size of data that write to this inode (data_)
  /// @buffer the src char-array
  /// @return On successe, return 0. On error, return -1.
    virtual int32 writeData(uint32 offset, uint32 size, const char *buffer);

    virtual void loadChildren();

  private:
    void writeZone(uint32 zone_number, char* buffer);
};

#endif // Inode_h___
