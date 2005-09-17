
//
// CVS Log Info for $RCSfile: PseudoFsSuperblock.h,v $
//
// $Id: PseudoFsSuperblock.h,v 1.2 2005/09/17 09:33:55 davrieb Exp $
// $Log: PseudoFsSuperblock.h,v $
// Revision 1.1  2005/09/16 16:27:40  davrieb
// add pseudofs
//
//
//

#ifndef PseudoFsSuperblock_h___
#define PseudoFsSuperblock_h___

#include "fs/ramfs/RamFsSuperblock.h"

#include "fs/PointList.h"
#include "fs/Superblock.h"

class Inode;
class Superblock;

class PseudoFsSuperblock : public RamFsSuperblock
{
 public:

  PseudoFsSuperblock(Dentry* s_root);

  virtual ~PseudoFsSuperblock();

  /// create a new Inode of the superblock, mknod with dentry, add in the list.
  virtual Inode* createInode(Dentry* dentry, uint32 mode);

};

#endif
