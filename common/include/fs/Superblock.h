/**
 * @file Superblock.h
 */

#ifndef SUPERBLOCK_H__
#define SUPERBLOCK_H__

#include "types.h"
#include "fs/PointList.h"
#include "StorageManager.h"

class Iattr;
class Statfs;
class WaitQueue;
class FileSystemType;
class VirtualFileSystem;
class FileDescriptor;

class Dentry;
class Inode;
class File;


/**
 * @class Superblock
 * The first block of the virtual-file-system. It contains for instance the
 * configuration of the file system. Source information concerning a mounted
 * filesystem. (For disk-based filesystems, this object usually corresponds
 * to a filesystem control block stored on disk.)
 */
class Superblock
{
  public:

    friend class VirtualFileSystem;

  protected:

    /**
     * The file system type.
     */
    const FileSystemType *s_type_;

    /**
     * The device that this file-system is mounted on.
     */
    uint32 s_dev_;

    /**
     * This records an identification number that has been read from the device
     * to confirm that the data on the device corresponds to the file-system
     */
    uint64 s_magic_;

    /**
     * This is a list of flags which are logically with the flags in each
     * inode to detemine certain behaviours. There is one flag which applies
     * only to the whole file-system.
     * exp: MS_RDONLY
     * A file-system with the flag set has been mounted read-only. No writing
     * be permitted, and no indirect modification, such as mount time in the
     * super-block or access times on files, will be made.
     */
    uint64 s_flags_;

    /**
     * The Dentry refers the root of the file-system. It is normally created by
     * loading the root inode from the file-system.
     */
    Dentry *s_root_;

    /**
     * The old Dentry of the mount point of a mounted file system
     */
    Dentry *mounted_over_;

    /**
     * A list of dirty inodes.
     */
    PointList<Inode> dirty_inodes_;

    /**
     * A list of used inodes. It is only used to open-file.
     */
    PointList<Inode> used_inodes_;

    /**
     * inodes of the superblock.
     */
    PointList<Inode> all_inodes_;

    /**
     * This is a list of files (linked on f_list) of open files on this
     * file-system. It is used, for example, to check if there are any files
     * open for write before remounting the file-system as read-only.
     */
    PointList<FileDescriptor> s_files_;

  public:

    /**
     * constructor
     * @param s_root the root dentry of the new filesystme
     * @param s_dev the device number of the new filesystem
     */
    Superblock ( Dentry* s_root, uint32 s_dev )
    {
      s_root_ = s_root;
      s_dev_ = s_dev;
    }

    /**
     * destructor
     */
    virtual ~Superblock();

    /**
     * create a new Inode of the superblock, mknod with dentry, add in the list.
     * @param dentry the dentry to create the inode with
     * @param type the inode type
     * @return the created inode
     */
    virtual Inode* createInode ( Dentry* /*dentry*/, uint32 /*type*/ ) { return 0; }

    /**
     * This method is called to read a specific inode from a mounted
     * file-system.
     * @param inode the inode to read
     * @return 0 on success
     */
    virtual int32 readInode ( Inode* /*inode*/ ) { return 0; }

    /**
     * This method is called to write a specific inode to a mounted file-system,
     * and gets called on inodes which have been marked dirty.
     * @param inode the inode to write
     */
    virtual void writeInode ( Inode* /*inode*/ ) {}

    /**
     * This method is called whenever the reference count on an inode is
     * decreased put_inode called before the i_count field is decreased, so if
     * put_inode wants to check if this is the last reference, it should check
     * if i_count is 1 or not. This method used it to do some special handling
     * when the last reference to the inode is release. i.e. when i_count is 1
     * and is about to be come zero.
     * @param inode the inode
     */
    virtual void put_inode ( Inode* /*inode*/ ) {}

    /**
     * This method is called whenever the reference count on an inode reaches 0,
     * and it is found that the link count (i_nlink= is also zero. It si
     * presumed that the file-system will deal with this situation be
     * invalidating the inode in the file-system and freeing up any resourses
     * used.
     * @param inode the inode to delete
     */
    virtual void delete_inode ( Inode* /*inode*/ );

    /**
     * This is called when inode attributed are changed, the argument class
     * Iattr* pointing to the new set of attributes. If the file-system does
     * not define this method (i.e. it is NULL) then VFS uses the routine
     * (inode_change_ok) which implements POSIX standard attributes
     * verification. Then VFS marks the inode as dirty. If the file-system
     * implements its own notify_change then it should call mark_inode_dirty
     * (Inode).
     * @param dentry the dentry to notify
     * @param iattr the changed attributes
     * @return 0 on success
     */
    virtual int32 notify_change ( Dentry* /*dentry*/, Iattr* /*iattr*/ ) { return 0; }

    /**
     * This method is called with super-block lock held. A typical
     * implementation would free file-system-private resources specific for
     * his mount instance, such as inode bitmaps, block bitmaps, a buffer
     * header containing super-block and decrement mount hold count if the
     * file-system is implemented as a dynamically loadable module.
     * Implementation: destructure of the Superblock.
     */
    virtual void put_super() {}

    /**
     * This method called when VFS decides that the super-block needs to be
     * written to disk. It check the SuperBlock->s_dirty_, if it is true write
     * the super block return to the Disc.
     */
    virtual void write_super() {}

    /**
     * This method is needed to implement statfs(2) system call
     */
    virtual int32 statfs ( Superblock*, Statfs*, int32 ) { return 0; }

    /**
     * This method called when file-system is being remounted, i.e. if the
     * MS_REMOUNT flag is specified with the mount system call. This can be used
     * to change various mount options without unmounting the file-system. A
     * common usage is to change a readonly file-system into a writable.
     */
    virtual int32 remount_fs ( Superblock*, int32*, char* ) { return 0; }

    /**
     * Optional method, call when VFS clears the inode. This is needed (at
     * least) by file-system which attaches kmalloced data to the inode
     * sturcture, as particularly might be the case for file-systems using the
     * generic_ip field in class Inode.
     * @param inode the inode to clear
     */
    virtual void clear_inode ( Inode* /*inode*/ ) {}

    /**
     * This method is called early in the unmounting process if the MNT_FORCE
     * flag was given to umount. The intentions is that it should cause any
     * incomplete transaction on the file-system to fail quickly rather than
     * block waiting on some external event such as a remote server responding.
     * @param superblock the superblock to unmount
     */
    virtual void umount_begin ( Superblock* /*super_block*/ ) {}

    /**
     * create a file with the given flag and  a file descriptor with the given
     * inode.
     * @param inode the inode to create the fd for
     * @param flag the flag
     * @return the fd
     */
    virtual int32 createFd ( Inode* /*inode*/, uint32 /*flag*/ ) {return 0;}

    /**
     * remove the corresponding file descriptor.
     * @param inode the indo from which to remove the fd
     * @param file the fd to remove
     * @return 0 on success
     */
    virtual int32 removeFd ( Inode* /*inode*/, FileDescriptor* /*file*/ ) { return 0;}

    /**
     * Get the root Dentry of the Superblock
     * @return the root dentry
     */
    Dentry *getRoot();

    /**
     * Get the mount point Dentry of the Superblock
     * @return the superblocks mount point dentry
     */
    Dentry *getMountPoint();

    /**
     * Get the File System Type of the Superblock
     * @return the file system type
     */
    FileSystemType *getFSType() {return ( FileSystemType * ) s_type_;}

};

#endif // Superblock_h___


