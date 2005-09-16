
//
// CVS Log Info for $RCSfile: PseudoFsSuperblock.h,v $
//
// $Id: PseudoFsSuperblock.h,v 1.1 2005/09/16 16:27:40 davrieb Exp $
// $Log$
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

};

#endif
