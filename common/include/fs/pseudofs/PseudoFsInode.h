

//
// CVS Log Info for $RCSfile: PseudoFsInode.h,v $
//
// $Id: PseudoFsInode.h,v 1.3 2005/09/28 20:11:36 qiangchen Exp $
// $Log: PseudoFsInode.h,v $
// Revision 1.2  2005/09/17 09:33:55  davrieb
// finished pseudofs code
//
// Revision 1.1  2005/09/16 16:27:40  davrieb
// add pseudofs
//
//
//

#ifndef PseudoFsInode_h___
#define PseudoFsInode_h___

#include "fs/ramfs/RamFsInode.h"

class PseudoFsInode : public RamFsInode
{

  public:

    PseudoFsInode(Superblock *super_block, uint32 inode_type);

    virtual ~PseudoFsInode();

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
    virtual int32 writeData(uint32 offset, uint32 size, char *buffer);

};

#endif
