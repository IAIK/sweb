// Projectname: SWEB
// Simple operating system for educational purposes

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
//  testPathWalker();
  kprintfd("***** begin testSyscallMkdir()\n");
  testSyscallMkdir();
  kprintfd("***** end testSyscallMkdir()\n");


  kprintfd("\n\n\n\n\n***** begin testSyscallReaddir()\n");
  testSyscallReaddir();
  kprintfd("***** end testSyscallReaddir()\n");

  kprintfd("\n\n\n\n\n***** begin testSyscallChdir()\n");
  testSyscallChdir();
  kprintfd("***** end testSyscallChdir()\n");
  
  kprintfd("\n\n\n\n\n***** begin testSyscallRmdir()\n");
  testSyscallRmdir();
  kprintfd("***** end testSyscallRmdir()\n");

//  testSyscallRmdirExtern();

  kprintfd("***** begin testUmount()\n");
  testUmount();
  kprintfd("***** end testUmount()\n");
}

//----------------------------------------------------------------------
void testSyscallMkdir()
{
  kprintfd("***** begin syscall mkdir(chen)\n");
  vfs_syscall.mkdir("chen", 0);
  kprintfd("***** end syscall mkdir(chen)\n");
  
  kprintfd("***** begin syscall mkdir(chen/qiang)\n");
  vfs_syscall.mkdir("/chen/qiang", 0);
  kprintfd("***** end syscall mkdir(chen/qiang)\n");

  kprintfd("***** begin syscall mkdir(chen/2006)\n");
  vfs_syscall.mkdir("/chen/2006", 0);
  kprintfd("***** end syscall mkdir(chen/2006)\n");

  kprintfd("***** begin syscall mkdir(chen/2005)\n");
  vfs_syscall.mkdir("/chen/2005", 0);
  kprintfd("***** end syscall mkdir(chen/2005)\n");

  kprintfd("***** begin syscall mkdir(chen/2007)\n");
  vfs_syscall.mkdir("/chen/2007", 0);
  kprintfd("***** end syscall mkdir(chen/2007)\n");

  kprintfd("***** begin syscall mkdir(chen/chen)\n");
  vfs_syscall.mkdir("chen/chen", 0);
  kprintfd("***** end syscall mkdir(chen/chen)\n");

  kprintfd("***** begin syscall mkdir(chen/chen/chen)\n");
  vfs_syscall.mkdir("chen/chen/chen", 0);
  kprintfd("***** end syscall mkdir(chen/chen/chen)\n");

  kprintfd("***** begin syscall mkdir(./.././chen/chen/chen/chen)\n");
  vfs_syscall.mkdir("./.././chen/chen/chen/chen", 0);
  kprintfd("***** end syscall mkdir(./.././chen/chen/chen/chen)\n");

  kprintfd("***** begin syscall mkdir(chen/chen/.././chen/chen/chen/.././chen/chen)\n");
  vfs_syscall.mkdir("chen/chen/.././chen/chen/chen/.././chen/chen", 0);
  kprintfd("***** end syscall mkdir(chen/chen/.././chen/chen/chen/.././chen/chen)\n");

  kprintfd("***** begin syscall mkdir(chen/chen/chen/chen/chen)\n");
  vfs_syscall.mkdir("chen/chen/chen/chen/chen", 0);
  kprintfd("***** end syscall mkdir(chen/chen/chen/chen/chen)\n");
}

//----------------------------------------------------------------------
void testSyscallReaddir()
{
  kprintfd("!!!!! begin syscall readdir(/)\n");
  vfs_syscall.readdir("/");
  kprintfd("!!!!! end syscall readdir(/)\n");

  kprintfd("!!!!! begin syscall readdir(chen)\n");
  vfs_syscall.readdir("chen");
  kprintfd("!!!!! end syscall readdir(chen)\n");

  kprintfd("!!!!! begin syscall readdir(../../../chen/../chen/./chen)\n");
  vfs_syscall.readdir("../../../chen/../chen/./chen");
  kprintfd("!!!!! end syscall readdir(../../../chen/../chen/./chen)\n");

  kprintfd("!!!!! begin syscall readdir(./chen/chen/chen)\n");
  vfs_syscall.readdir("./chen/chen/chen");
  kprintfd("!!!!! end syscall readdir(./chen/chen/chen)\n");

  kprintfd("!!!!! begin syscall readdir(./chen/chen/./chen)\n");
  vfs_syscall.readdir("./chen/chen/chen/./chen");
  kprintfd("!!!!! end syscall readdir(./chen/chen/./chen)\n");

  kprintfd("!!!!! begin syscall readdir(./chen/chen/chen/chen/chen)\n");
  vfs_syscall.readdir("./chen/chen/chen/chen/chen");
  kprintfd("!!!!! end syscall readdir(./chen/chen/chen/chen/chen)\n");
  
  kprintfd("!!!!! begin syscall readdir(/chen/chen/qiang)\n");
  vfs_syscall.readdir("/chen/chen/qiang");
  kprintfd("!!!!! end syscall readdir(/chen/chen/qiang)\n");
}

//----------------------------------------------------------------------
void testSyscallChdir()
{
  kprintfd("!!!!! begin syscall chdir(/chen)\n");
  vfs_syscall.chdir("/chen");
  vfs_syscall.readdir(".");
  kprintfd("!!!!! end syscall chdir(/chen)\n");

  kprintfd("!!!!! begin syscall chdir(chen)\n");
  vfs_syscall.chdir("chen");
  vfs_syscall.readdir(".");
  kprintfd("!!!!! end syscall chdir(chen)\n");

  kprintfd("!!!!! begin syscall chdir(../../../chen/../chen/./chen)\n");
  vfs_syscall.chdir("../../../chen/../chen/./chen");
  vfs_syscall.readdir(".");
  kprintfd("!!!!! end syscall chdir(../../../chen/../chen/./chen)\n");

  kprintfd("!!!!! begin syscall chdir(./chen/chen/chen)\n");
  vfs_syscall.chdir("./chen/chen/chen");
  vfs_syscall.readdir(".");
  kprintfd("!!!!! end syscall chdir(./chen/chen/chen)\n");

  kprintfd("!!!!! begin syscall chdir(./chen/chen/./chen)\n");
  vfs_syscall.chdir("./chen/chen/chen/./chen");
  vfs_syscall.readdir(".");
  kprintfd("!!!!! end syscall chdir(./chen/chen/./chen)\n");

  kprintfd("!!!!! begin syscall chdir(./chen/chen/chen/chen/chen)\n");
  vfs_syscall.chdir("./chen/chen/chen/chen/chen");
  vfs_syscall.readdir(".");
  kprintfd("!!!!! end syscall chdir(./chen/chen/chen/chen/chen)\n");
  
  kprintfd("!!!!! begin syscall chdir(/chen/chen/qiang)\n");
  vfs_syscall.chdir("/chen/chen/qiang");
  vfs_syscall.readdir(".");
  kprintfd("!!!!! end syscall chdir(/chen/chen/qiang)\n");
}

//----------------------------------------------------------------------
void testSyscallRmdir()
{
  kprintfd("***** begin syscall rmdir(chen)\n");
  vfs_syscall.rmdir("chen");
  kprintfd("***** end syscall rmdir(chen)\n");
  
  kprintfd("***** begin syscall rmdir(chen)\n");
  vfs_syscall.rmdir("/chen/qiang");
  kprintfd("***** end syscall rmdir(chen)\n");

  kprintfd("***** begin syscall rmdir(chen)\n");
  vfs_syscall.rmdir("/chen/2006");
  kprintfd("***** end syscall rmdir(chen)\n");

  kprintfd("***** begin syscall rmdir(chen)\n");
  vfs_syscall.rmdir("/chen/2005");
  kprintfd("***** end syscall rmdir(chen)\n");

  kprintfd("***** begin syscall rmdir(chen)\n");
  vfs_syscall.rmdir("/chen/2007");
  kprintfd("***** end syscall rmdir(chen)\n");

  kprintfd("***** begin syscall rmdir(chen/chen)\n");
  vfs_syscall.rmdir("chen/chen");
  kprintfd("***** end syscall rmdir(chen/chen)\n");

  kprintfd("***** begin syscall rmdir(chen/chen)\n");
  vfs_syscall.rmdir("chen/chen/chen");
  kprintfd("***** end syscall rmdir(chen/chen)\n");

  kprintfd("***** begin syscall rmdir(chen/chen)\n");
  vfs_syscall.rmdir("./.././chen/chen/chen/chen");
  kprintfd("***** end syscall rmdir(chen/chen)\n");

  kprintfd("***** begin syscall rmdir(chen/chen)\n");
  vfs_syscall.rmdir("chen/chen/.././chen/chen/chen/.././chen/chen");
  kprintfd("***** end syscall rmdir(chen/chen)\n");

  kprintfd("***** begin syscall rmdir(chen/chen)\n");
  vfs_syscall.rmdir("chen/chen/chen/chen/chen");
  kprintfd("***** end syscall rmdir(chen/chen)\n");
}

//----------------------------------------------------------------------
void testSyscallRmdirExtern()
{
  vfs_syscall.mkdir("chen", 0);
  vfs_syscall.chdir("chen");
  vfs_syscall.mkdir("chen", 0);
  vfs_syscall.chdir("chen");
  vfs_syscall.mkdir("chen", 0);
  vfs_syscall.chdir("chen");
  vfs_syscall.mkdir("chen", 0);
  vfs_syscall.chdir("chen");
  vfs_syscall.mkdir("chen", 0);
  vfs_syscall.chdir("chen");
  vfs_syscall.chdir("..");
  vfs_syscall.rmdir("chen");
  vfs_syscall.chdir("..");
  vfs_syscall.rmdir("chen");
  vfs_syscall.chdir("..");
  vfs_syscall.rmdir("chen");
  vfs_syscall.chdir("..");
  vfs_syscall.rmdir("chen");
  vfs_syscall.chdir("..");
  vfs_syscall.rmdir("chen");
}

//----------------------------------------------------------------------
void testMini()
{
  vfs_syscall.mkdir("chen",0);
  vfs_syscall.mkdir("qiang",0);
  vfs_syscall.readdir(".");
  vfs_syscall.chdir("qiang");
  vfs_syscall.readdir("..");
}

//----------------------------------------------------------------------
void testMount()
{
  RamFileSystemType *ramfs = new RamFileSystemType();
  vfs.registerFileSystem(ramfs);
  
  vfs.root_mount("ramfs", 0);
}

//----------------------------------------------------------------------
void testUmount()
{
  vfs.rootUmount();
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
  
  kprintfd("***** start of mkdir(./hugo)\n");  
  vfs_syscall.mkdir("./hugo", 0);
  kprintfd("***** end of mkdir(./hugo)\n");  

  kprintfd("***** start of mkdir(test)\n");  
  vfs_syscall.mkdir("./../chen/.././SSS", 0);
  kprintfd("***** end of mkdir(test)\n");  

  kprintfd("***** start of mkdir(../hugo)\n");  
  vfs_syscall.mkdir("../hugo", 0);
  kprintfd("***** end of mkdir(../hugo)\n");  
  

  kprintfd("***** start of readdir(/)\n");
  vfs_syscall.readdir("/");
  kprintfd("***** end of readdir(/)\n");

  kprintfd("***** start of readdir(/)\n");
  vfs_syscall.readdir("/chen");
  kprintfd("***** end of readdir(/)\n");
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
