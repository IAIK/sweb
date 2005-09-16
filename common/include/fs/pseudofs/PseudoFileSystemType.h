
#ifndef PSEUDOFILESYSTEMTYPE_H__
#define PSEUDOFILESYSTEMTYPE_H__

#include "fs/ramfs/RamFileSystemType.h"

class PseudoFileSystemType : public  RamFileSystemType
{
  public:

    PseudoFileSystemType();

    virtual ~PseudoFileSystemType();

    virtual Superblock *readSuper(Superblock *superblock, void* data) const;

    virtual Superblock *createSuper(Dentry *root) const;

};

#endif
