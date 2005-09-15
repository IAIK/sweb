// Projectname: SWEB
// Simple operating system for educational purposes

#ifndef RamFsSuperblock_h___
#define RamFsSuperblock_h___

#include "fs/PointList.h"
#include "fs/Superblock.h"

class Inode;
class Superblock;

//-----------------------------------------------------------------------------
/**
 * RamFsSuperblock
 *
 */
class RamFsSuperblock : public Superblock
{
 public:

  RamFsSuperblock(Dentry* s_root);

  virtual ~RamFsSuperblock();

  /// create a new Inode of the superblock, mknod with dentry, add in the list.
  virtual Inode* createInode(Dentry* dentry, uint32 mode);

  /// This method is called to read a specific inode from a mounted
  /// file-system.
  virtual void read_inode(Inode* inode);

  /// This method is called to write a specific inode to a mounted file-system,
  /// and gets called on inodes which have been marked dirty.
  virtual void write_inode(Inode* inode);

  /// This method is called whenever the reference count on an inode reaches 0,
  /// and it is found that the link count (i_nlink= is also zero. It si
  /// presumed that the file-system will deal with this situation be
  /// invalidating the inode in the file-system and freeing up any resourses
  /// used.
  virtual void delete_inode(Inode* inode);
};
//-----------------------------------------------------------------------------

#endif // RamFsSuperblock_h___
