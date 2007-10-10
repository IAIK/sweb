/**
 * @file PseudoFSSuperblock.h
 */

#ifndef PseudoFsSuperblock_h___
#define PseudoFsSuperblock_h___

#include "fs/ramfs/RamFSSuperblock.h"

#include "fs/PointList.h"
#include "fs/Superblock.h"

class Inode;
class Superblock;

/**
 * @class PseudoFSSuperblock
 */
class PseudoFSSuperblock : public RamFSSuperblock
{
  public:

    /**
     * constructor
     * @param s_root the root dentry of the new filesystem
     * @param s_dev the device number of the new filesystem
     */
    PseudoFSSuperblock ( Dentry* s_root, uint32 s_dev );

    /**
     * destructor
     */
    virtual ~PseudoFSSuperblock();

    /**
     * creates a new Inode of the superblock, mknod with dentry, add in the list.
     * @param dentry the dentry to create the inode with
     * @param type the inode type
     * @return the inode
     */
    virtual Inode* createInode ( Dentry* dentry, uint32 type );

};

#endif
