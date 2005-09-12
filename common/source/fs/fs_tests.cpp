
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
  
//  testVfsSyscall();
  testPathWalker();
  
  kprintfd("***** begin testUmount()\n");
  testUmount();
  kprintfd("***** begin testUmount()\n");
}

//----------------------------------------------------------------------
void testMount()
{
  RamFileSystemType *ramfs = new RamFileSystemType();
  kprintfd("testMount 1\n");
  vfs.registerFileSystem(ramfs);
  kprintfd("testMount 2\n");
  
  vfs.root_mount("ramfs", 0);
  kprintfd("testMount 3\n");
}

//----------------------------------------------------------------------
void testUmount()
{
  vfs.rootUmount("ramfs");
}

//----------------------------------------------------------------------
void testPathWalker()
{
  kprintfd("***** start of mdkir(/chen)\n");
  vfs_syscall.mkdir("/chen", 0);
  kprintfd("***** end of mkdir(/chen)\n");

  kprintfd("***** start of mkdir(test)\n");  
  vfs_syscall.mkdir("test", 0);
  kprintfd("***** end of mkdir(test)\n");  
  
  kprintfd("***** start of chdir(chen)\n");
  vfs_syscall.chdir("chen");
  kprintfd("***** end of chdir(chen)\n");
  
  kprintfd("***** start of mkdir(test)\n");  
  vfs_syscall.mkdir("./../chen/.././SSS", 0);
  kprintfd("***** end of mkdir(test)\n");  

  kprintfd("***** start of mkdir(../hugo)\n");  
  vfs_syscall.mkdir("../hugo", 0);
  kprintfd("***** end of mkdir(../hugo)\n");  
  
  kprintfd("***** start of mkdir(./hugo)\n");  
  vfs_syscall.mkdir("./hugo", 0);
  kprintfd("***** end of mkdir(./hugo)\n");  

  kprintfd("***** start of readdir(/)\n");
  vfs_syscall.readdir("/");
  kprintfd("***** end of readdir(/)\n");

  kprintfd("***** start of readdir(/)\n");
  vfs_syscall.readdir("/chen");
  kprintfd("***** end of readdir(/)\n");
/*  
  kprintfd("***** start of mkdir(auto)\n");
  vfs_syscall.mkdir("auto", 0);
  kprintfd("***** end of mkdir(auto)\n");
  
  kprintfd("***** start of readdir(/)\n");
  vfs_syscall.readdir("/");
  kprintfd("***** end of readdir(/)\n");
  
  kprintfd("***** start of readdir(/chen)\n");
  vfs_syscall.readdir("/chen");
  kprintfd("***** end of readdir(/chen)\n");
*/
/*  
  kprintfd("***** start of mkdir(/chen/qiang)\n");
  vfs_syscall.mkdir("/chen/qiang", 0);
  kprintfd("***** end of mkdir(/chen/qiang)\n");
  
  kprintfd("***** start of mkdir(/chen/qiang/always)\n");
  vfs_syscall.mkdir("/chen/qiang/always", 0);
  kprintfd("***** end of mkdir(/chen/qiang/always)\n");
  
  kprintfd("***** start of mkdir(/chen/2005)\n");
  vfs_syscall.mkdir("/chen/2005", 0);
  kprintfd("***** end of mkdir(/chen/2005)\n");
  
  kprintfd("***** start of readdir(/chen)\n");
  vfs_syscall.readdir("/chen/");
  kprintfd("***** end of readdir(/chen)\n");
  
  kprintfd("***** start of mkdir(/chen/qiang/always/dead)\n");
  vfs_syscall.mkdir("/chen/qiang/always/dead", 0);
  kprintfd("***** end of mkdir(/chen/qiang/always/dead)\n");
  
  kprintfd("***** start of chdir(/chen/qiang)");
  vfs_syscall.chdir("/chen/qiang");
  kprintfd("***** end of chdir(/chen/qiang)");
  */
}

//----------------------------------------------------------------------
void testVfsSyscall()
{
  kprintfd("***** start of mdkir(/chen)\n");
  vfs_syscall.mkdir("/chen", 0);
  kprintfd("***** end of mkdir(/chen)\n");
  
  kprintfd("***** start of mkdir(/chen/qiang)\n");
  vfs_syscall.mkdir("/chen/qiang", 0);
  kprintfd("***** end of mkdir(/chen/qiang)\n");
  
  kprintfd("***** start of mkdir(/chen/qiang/always)\n");
  vfs_syscall.mkdir("/chen/qiang/always", 0);
  kprintfd("***** end of mkdir(/chen/qiang/always)\n");
  
  kprintfd("***** start of mkdir(/chen/2005)\n");
  vfs_syscall.mkdir("/chen/2005", 0);
  kprintfd("***** end of mkdir(/chen/2005)\n");
  
  kprintfd("***** start of readdir(/chen)\n");
  vfs_syscall.readdir("/chen/");
  kprintfd("***** end of readdir(/chen)\n");
  
  kprintfd("***** start of chdir(/chen/qiang)\n");
  vfs_syscall.chdir("/chen/qiang/");
  kprintfd("***** end of chdir(/chen/qiang)\n");
  
  kprintfd("***** start of mkdir(/chen/qiang/always/dead)\n");
  vfs_syscall.mkdir("/chen/qiang/always/dead", 0);
  kprintfd("***** end of mkdir(/chen/qiang/always/dead)\n");
  
  kprintfd("***** start of rmdir(/chen/qiang/always/dead)\n");
  vfs_syscall.rmdir("/chen/qiang/always/dead");
  kprintfd("***** end of rmdir(/chen/qiang/always/dead)\n");
}
