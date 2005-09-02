
#include "fs/fs_tests.h"


#include "fs/ramfs/RamFileSystemType.h"
#include "fs/VirtualFileSystem.h"
#include "fs/PathWalker.h"

#ifndef STANDALONE
#include "console/kprintf.h"
#endif

#include "fs/fs_global.h"

//------------------------------------------------------------
// testing the registerfilesystem
void testRegFS()
{
  testMount();
}


//----------------------------------------------------------------------
void testMount()
{
  RamFileSystemType *ramfs = new RamFileSystemType();
  vfs.registerFileSystem(ramfs);

  vfs.root_mount("ramfs", 0);
}


//----------------------------------------------------------------------
void testPathWalker()
{
}

