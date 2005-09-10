
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
  kprintfd("***** begin testMount()\n");
  testMount();
  kprintfd("***** end testMount()\n");
  
  testPathWalker();
}


//----------------------------------------------------------------------
void testMount()
{
  RamFileSystemType *ramfs = new RamFileSystemType();
  kprintfd("I'm hier\n");
  vfs.registerFileSystem(ramfs);
  kprintfd("I'm hier2\n");
  
  vfs.root_mount("ramfs", 0);
  kprintfd("I'm hier3\n");
}


//----------------------------------------------------------------------
void testPathWalker()
{
  kprintfd("***** begin testPathWalker()\n");
  int32 success = path_walker.pathInit("/", 0);
  kprintfd("after pathInit() success = %d\n", success);
  if(success == 0)
    success = path_walker.pathWalk("/");
  kprintfd("after pathWalk() success = %d\n", success);
  path_walker.pathRelease();
  kprintfd("***** end testPathWalker()\n");
}

