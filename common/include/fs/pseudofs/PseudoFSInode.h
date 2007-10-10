/**
 * @file PseudoFSInode.h
 */
#ifndef PSEUDO_FS_INODE_H__
#define PSEUDO_FS_INODE_H__

#include "fs/ramfs/RamFSInode.h"

class PseudoFSInode : public RamFSInode
{

  public:

    /**
     * constructor
     * @param super_block the superblock on which the inode should be created
     * @param inode_type the inode type
     */
    PseudoFSInode ( Superblock *super_block, uint32 inode_type );

    /**
     * destructor
     */
    virtual ~PseudoFSInode();

    /**
     * read the data from the inode
     * @param offset offset byte
     * @param size the size of data that read from this inode
     * @param buffer the dest char-array to store the data
     * @return On successe, return 0. On error, return -1.
     */
    virtual int32 readData ( uint32 offset, uint32 size, char *buffer );

    /**
     * write the data to the inode
     * @param offset offset byte
     * @param size the size of data that write to this inode (data_)
     * @param buffer the src char-array
     * @return On successe, return 0. On error, return -1.
     */
    virtual int32 writeData ( uint32 offset, uint32 size, char *buffer );

};

#endif
