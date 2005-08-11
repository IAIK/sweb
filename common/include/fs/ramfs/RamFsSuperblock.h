
//
// CVS Log Info for $RCSfile: RamFsSuperblock.h,v $
//
// $Id: RamFsSuperblock.h,v 1.5 2005/08/11 16:34:28 qiangchen Exp $
// $Log: RamFsSuperblock.h,v $
// Revision 1.4  2005/07/21 18:07:03  davrieb
// mount of the root directory
//
// Revision 1.3  2005/07/16 13:22:00  davrieb
// rrename List in fs to PointList to avoid name clashes
//
// Revision 1.2  2005/07/07 12:31:19  davrieb
// add ramfs and all changes it caused
//
// Revision 1.1  2005/06/01 09:20:36  davrieb
// add all changes to fs
//
// Revision 1.1  2005/05/10 16:42:32  davrieb
// add first attempt to write a virtual file system
//
//

#ifndef RamFsSuperblock_h___
#define RamFsSuperblock_h___

#include "fs/PointList.h"
#include "fs/Superblock.h"
#include "fs/ramfs/RamFsInode.h"

class Superblock;

//-----------------------------------------------------------------------------
/**
 * RamFsSuperblock
 *
 * The first block of the virtual-file-system. It contains for instance the
 * configuration of the file system. Sotre information concerning a mounted
 * filesystem. For disk-based filesystems, this object usually corresponds
 * to a filesystem control block stored on disk.
 */
class RamFsSuperblock : public Superblock
{

protected:
  PointList<Inode> all_inodes_;

public:

  RamFsSuperblock(Dentry* s_root) :
    Superblock(s_root)
  {

    Dentry *root_dentry = 0;

    if (s_root)
    {
      Dentry* parent = s_root->get_parent();
      root_dentry = new Dentry(parent);

      mounted_over_ = s_root;
      s_root = root_dentry;
    }
    else
    {
      root_dentry = new Dentry(0);
    }

    Inode *root_inode = (Inode*)(new RamFsInode(this, I_DIR));
    root_dentry->set_inode(root_inode);

    all_inodes_.push_end(root_inode);
  }

  virtual ~RamFsSuperblock();

  /// This method is called to read a specific inode from a mounted
  /// file-system. It is only called from get_new_inode.
  virtual void read_inode(Inode* inode);

  /// This method is called to write a specific inode to a mounted file-system,
  /// and gets called on inodes which have been marked dirty with
  /// mark_inode_dirty.
  virtual void write_inode(Inode* inode);

  /// This method is called whenever the reference count on an inode reaches 0,
  /// and it is found that the link count (i_nlink= is also zero. It si
  /// presumed that the file-system will deal with this situation be
  /// invalidating the inode in the file-system and freeing up any resourses
  /// used.
  virtual void delete_inode(Inode* inode);

  /// This method is needed to implement statfs(2) system call
  virtual int32 statfs(Superblock*, Statfs*, int32) { return 0; }

  /// Optional method, call when VFS clears the inode. This is needed (at
  /// least) by file-system which attaches kmalloced data to the inode
  /// sturcture, as particularly might be the case for file-systems using the
  /// generic_ip field in class Inode.
  virtual void clear_inode(Inode* inode) {}
  
};
//-----------------------------------------------------------------------------

#endif // Superblock_h___
