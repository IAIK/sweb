/**
 * @file PseudoFSType.h
 */

#ifndef PSEUDO_FS_TYPE_H__
#define PSEUDO_FS_TYPE_H__

#include "fs/ramfs/RamFSType.h"

/**
 * @class PseudoFSType
 */
class PseudoFSType : public  RamFSType
{
  public:

    /**
     * constructor
     */
    PseudoFSType();

    /**
     * destructor
     */
    virtual ~PseudoFSType();

    /**
     * returns the given superblock
     * @param superblock the superblock
     * @param data not used
     * @return the superblock
     */
    virtual Superblock *readSuper ( Superblock *superblock, void* data ) const;

    /**
     * creates a new superblock
     * @param root the root dentry for the new superblock
     * @param s_dev the device name for the new superblock
     * @return the superblock
     */
    virtual Superblock *createSuper ( Dentry *root, uint32 s_dev ) const;

};

#endif
