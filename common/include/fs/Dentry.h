/**
 * @file Dentry.h
 */
#include "types.h"
#include <ustl/ulist.h>
#include "kstring.h"
#include <ustl/ualgo.h>

#ifndef DENTRY_h__
#define DENTRY_h__

class Inode;

/**
 * @class Dentry
 * The VFS layer does all management of path names of files, and converts them
 * into entries in the dentry before passing allowing the underlying
 * file-system to see them. The dentry object associates the component to its
 * corresponding inode.
 */
class Dentry
{
  protected:
    friend class MinixFSInode;
    friend class VfsSyscall;
    /**
     * The pointer to the inode related to this name.
     */
    Inode *d_inode_;

    /**
     * This will point to the parent dentry. For the root of a file-system, or
     * for an anonymous entry like that for a file, this points back to the
     * containing dentry itself.
     */
    Dentry *d_parent_;

    /**
     * This list_head is used to link together all the children of the dentry.
     */
    ustl::list<Dentry*> d_child_;

    /**
     * For a directory that has had a file-system mounted on it, this points to
     * the mount point of that current file-system. For other dentries, this
     * points back to the dentry itself.
     */
    Dentry *d_mounts_;

    /**
     * The d_name field contains the name of this entry, together with its hash
     * value.
     */
    char *d_name_;

  public:

    /**
     * set the inode to the dentry
     * @param inode the inode to set
     */
    void setInode(Inode *inode);

    /**
     * release the inode to the dentry
     */
    void releaseInode()
    {
      d_inode_ = 0;
    }

    /**
     * get the inode to dentry
     * @return the inode
     */
    Inode* getInode()
    {
      return d_inode_;
    }

    /**
     * return the parent of the dentry
     * @return the dentry
     */
    Dentry* getParent()
    {
      return d_parent_;
    }

    /**
     * set the parent dentry
     * @param parent the parent dentry to set
     */
    void setParent(Dentry *parent)
    {
      d_parent_ = parent;
    }

    /**
     * return the mount_point of the current file-system
     * @return the dentry of the mount point
     */
    Dentry* getMountPoint()
    {
      return d_mounts_;
    }

    /**
     * set the mount point
     * @param mount_point the dentry to set the mount point to
     */
    void setMountPoint(Dentry *mount_point)
    {
      d_mounts_ = mount_point;
    }

    /**
     * set the child to the dentry
     * @param dentry the child dentry to set
     * @return 0 on success
     */
    int32 setChild(Dentry *dentry);

    /**
     * check the existance of the child-list
     * @return true is empty
     */
    bool emptyChild();

    /**
     * get the number of the child
     * @return the number of childs
     */
    uint32 getNumChild();

    /**
     * get the child of the child-list
     * @param indes the index of the child to get
     * @return the found child dentry
     */
    Dentry* getChild(uint32 index);

    /**
     * set the name to the dentry
     * @param name the name to set
     */
    void setName(const char* name);

    /**
     * return the name of the dentry
     * @return the dentry's name
     */
    char* getName();

    /**
     * This should compare the name with the all names of the d_child_ list.
     * It should return the Dentry if it exists the same name in the list,
     * @return the dentry found, 0 if doesn't exist.
     */
    virtual Dentry* checkName(const char* name);

    /**
     * remove a child_dentry from the d_child_ list.
     * @param child_dentry the child dentry of the curent dentry.
     * @return 0 on success
     */
    virtual int32 childRemove(Dentry *child_dentry);

    /**
     * insert a child dentry to the d_child_ list.
     * @param child_dentry the child dentry of the current dentry.
     */
    virtual void childInsert(Dentry *child_dentry);

  public:

    /**
     * Constructor of a new dentry.
     * construct a new dentry with the name,  the root-directory call this
     * contructor.
     * @param name the name
     */
    Dentry(const char* name);

    /**
     * Constructor of a new dentry.
     * It muss to check the double name in the parent dentry before to call this
     * contructor.
     * @param parent the parent
     */
    Dentry(Dentry *parent);

    /**
     * Delete the pointer in all parent-dentrys before to calling this
     * destructor
     */
    virtual ~Dentry();
};

#endif // Dentry_h___
