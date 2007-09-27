/**
 * @file fs_test.h
 */

#include "fs/fs_tests.h"
#include "types.h"

#include "fs/ramfs/RamFSType.h"
#include "fs/pseudofs/PseudoFSType.h"
#include "fs/VirtualFileSystem.h"
#include "fs/PathWalker.h"

#include "console/kprintf.h"

#include "fs/fs_global.h"


// testing the registerfilesystem
void testRegFS()
{
  kprintfd ( "***** begin ROOT_Mount()\n" );
  RamFSType *ramfs = new RamFSType();
  vfs.registerFileSystem ( ramfs );
  vfs.root_mount ( "ramfs", 0 );
  kprintfd ( "***** end ROOT_Mount()\n" );

  //testMountUmount();
  //testFile();
  testFinal();

  kprintfd ( "***** begin ROOTUmount()\n\n\n" );
  vfs.rootUmount();
  vfs.unregisterFileSystem ( ramfs );
  kprintfd ( "***** end ROOTUmount()\n" );
}

//----------------------------------------------------------------------
void testFinal()
{
  kprintfd ( "\n> mkdir dev\n" );
  vfs_syscall.mkdir ( "dev",0 );

  kprintfd ( "\n> open ../dev/hda0.txt\n" );
  int32 hda0_fd = vfs_syscall.open ( "../dev/hda0.txt", 2 );

  kprintfd ( "\n> write hallo world! to hda0.txt" );
  vfs_syscall.write ( hda0_fd, "hallo world!", 12 );

  kprintfd ( "\n> rmdir dev\n" );
  vfs_syscall.rmdir ( "dev" );

  kprintfd ( "\n> mkdir boot\n" );
  vfs_syscall.mkdir ( "/../../dev/../../boot", 0 );

  kprintfd ( "\n> mkdir usr\n" );
  vfs_syscall.mkdir ( "usr", 0 );

  kprintfd ( "\n> rmdir /dev/hda0.txt\n" );
  vfs_syscall.rmdir ( "/dev/hda0.txt" );

  kprintfd ( "\n> rm /dev/hda0.txt\n" );
  vfs_syscall.rm ( "dev/hda0.txt" );

  kprintfd ( "\n> write hallo world! to hda0.txt" );
  char cbuffer[20];
  int32 size = vfs_syscall.read ( hda0_fd, cbuffer, 12 );
  cbuffer[size] = '\0';
  kprintfd ( "cbuffer = %s\n", cbuffer );

  kprintfd ( "\n> close /dev/hda0.txt\n" );
  vfs_syscall.close ( hda0_fd );

  kprintfd ( "\n> rmdir /dev/hda0.txt\n" );
  vfs_syscall.rmdir ( "/dev/hda0.txt" );

  kprintfd ( "\n> rm /dev/hda0.txt\n" );
  vfs_syscall.rm ( "dev/hda0.txt" );

  kprintfd ( "\n> rm /dev\n" );
  vfs_syscall.rm ( "/dev" );

}

//----------------------------------------------------------------------
void testFile()
{
  kprintfd ( "\n> open test01.txt\n" );
  int32 test01_fd = vfs_syscall.open ( "test01.txt", 0 );

  kprintfd ( "\n> open test02.hugo\n" );
  int32 test02_fd = vfs_syscall.open ( "test02.hugo", 2 );

  kprintfd ( "\n> close test01.txt\n" );
  vfs_syscall.close ( test01_fd );

  kprintfd ( "\n> write hallo world! to test02.hugo\n" );
  vfs_syscall.write ( test02_fd, "hallo world!", 12 );

  kprintfd ( "\n> read 12 byte from the test02.hugo\n" );
  char test_array[20];
  int32 size = vfs_syscall.read ( test02_fd, test_array, 12 );
  test_array[size] = '\0';
  kprintfd ( "size = %d\n", size );
  kprintfd ( "after read-syscall test_array = %s\n", test_array );

  kprintfd ( "\n> close test02.hugo\n" );
  vfs_syscall.close ( test02_fd );

  kprintfd ( "\n> open test02.hugo READ_ONLY\n" );
  test02_fd = vfs_syscall.open ( "test02.hugo", 0 );

  kprintfd ( "\n> open test02.hugo\n" );
  vfs_syscall.open ( "test02.hugo", 0 );

  kprintfd ( "\n> open test02.hugo\n" );
  vfs_syscall.open ( "test02.hugo", 0 );

  kprintfd ( "\n> open test02.hugo\n" );
  vfs_syscall.open ( "test02.hugo", 0 );

  kprintfd ( "\n> read 12 byte from the test02.hugo\n" );
  char test2_array[20];
  int32 size2 = vfs_syscall.read ( test02_fd, test2_array, 12 );
  test2_array[size2] = '\0';
  kprintfd ( "size = %d\n", size2 );
  kprintfd ( "after read-syscall test2_array = %s\n", test2_array );
}

//----------------------------------------------------------------------
void testMountUmount()
{
  kprintfd ( "\n> mkdir var\n" );
  vfs_syscall.mkdir ( "var", 0 );

  kprintfd ( "\n> mkdir ../../var\n" );
  vfs_syscall.mkdir ( "../../var", 0 );

  kprintfd ( "\n> mkdir /dev\n" );
  vfs_syscall.mkdir ( "/dev", 0 );

  kprintfd ( "\n> mkdir ../root\n" );
  vfs_syscall.mkdir ( "../root",0 );

  kprintfd ( "\n> mkdir ../var/../dev/../root/../dev/hda\n" );
  vfs_syscall.mkdir ( "../var/../dev/../root/../dev/hda", 0 );

  kprintfd ( "\n> ls .\n" );
  vfs_syscall.readdir ( "." );

  kprintfd ( "\n> cd hda\n" );
  vfs_syscall.chdir ( "hda" );

  kprintfd ( "\n> cd dev/hda\n" );
  vfs_syscall.chdir ( "dev/hda" );

  kprintfd ( "\n> mkdir hugo\n" );
  vfs_syscall.mkdir ( "hugo", 0 );

  kprintfd ( "\n> ls .\n" );
  vfs_syscall.readdir ( "." );

  kprintfd ( "\nmount a new ramfs in the directory: /dev/hda\n" );
  vfs.mount ( 0, "/dev/hda", "ramfs", 0 );

  kprintfd ( "\n> ls .\n" );
  vfs_syscall.readdir ( "." );

  kprintfd ( "\n> cd /dev/hda\n" );
  vfs_syscall.chdir ( "/dev/hda" );

  kprintfd ( "\n> ls .\n" );
  vfs_syscall.readdir ( "." );

  kprintfd ( "\n> mkdir /dev/hda/win_C\n" );
  vfs_syscall.mkdir ( "/dev/hda/win_C", 0 );

  kprintfd ( "\n> mkdir win_D\n" );
  vfs_syscall.mkdir ( "win_D", 0 );

  kprintfd ( "\n> mkdir ../auto\n" );
  vfs_syscall.mkdir ( "../auto", 0 );

  kprintfd ( "\n> mkdir win_C/matlab\n" );
  vfs_syscall.mkdir ( "win_C/matlab", 0 );

  kprintfd ( "\n> ls .\n" );
  vfs_syscall.readdir ( "." );

  kprintfd ( "\n> ls ..\n" );
  vfs_syscall.readdir ( ".." );

  kprintfd ( "\n> rmdir win_C\n" );
  vfs_syscall.rmdir ( "win_C" );

  kprintfd ( "\n> rmdir win_D\n" );
  vfs_syscall.rmdir ( "win_D" );

  kprintfd ( "\n> ls .\n" );
  vfs_syscall.readdir ( "." );

  kprintfd ( "\numount the ramfs\n" );
  vfs.umount ( "/dev/hda", 0 );

  kprintfd ( "\n> ls .\n" );
  vfs_syscall.readdir ( "." );

  kprintfd ( "\n> mkdir dino\n" );
  vfs_syscall.mkdir ( "dino", 0 );

  kprintfd ( "\n> ls .\n" );
  vfs_syscall.readdir ( "." );
}
//----------------------------------------------------------------------
void testSyscallMkdir()
{
  kprintfd ( "***** begin syscall mkdir(chen)\n" );
  vfs_syscall.mkdir ( "chen", 0 );
  kprintfd ( "***** end syscall mkdir(chen)\n" );

  kprintfd ( "***** begin syscall mkdir(chen/2005)\n" );
  vfs_syscall.mkdir ( "/chen/2005", 0 );
  kprintfd ( "***** end syscall mkdir(chen/2005)\n" );

  kprintfd ( "***** begin syscall mkdir(chen/chen)\n" );
  vfs_syscall.mkdir ( "chen/chen", 0 );
  kprintfd ( "***** end syscall mkdir(chen/chen)\n" );

  kprintfd ( "***** begin syscall mkdir(chen/chen/chen)\n" );
  vfs_syscall.mkdir ( "chen/chen/chen", 0 );
  kprintfd ( "***** end syscall mkdir(chen/chen/chen)\n" );

  kprintfd ( "***** begin syscall mkdir(./.././chen/chen/chen/chen)\n" );
  vfs_syscall.mkdir ( "./.././chen/chen/chen/chen", 0 );
  kprintfd ( "***** end syscall mkdir(./.././chen/chen/chen/chen)\n" );

  kprintfd ( "***** begin syscall mkdir(chen/chen/.././chen/chen/chen/.././chen/chen)\n" );
  vfs_syscall.mkdir ( "chen/chen/.././chen/chen/chen/.././chen/chen", 0 );
  kprintfd ( "***** end syscall mkdir(chen/chen/.././chen/chen/chen/.././chen/chen)\n" );

  kprintfd ( "***** begin syscall mkdir(chen/chen/chen/chen/chen)\n" );
  vfs_syscall.mkdir ( "chen/chen/chen/chen/chen", 0 );
  kprintfd ( "***** end syscall mkdir(chen/chen/chen/chen/chen)\n" );
}

//----------------------------------------------------------------------
void testSyscallReaddir()
{
  kprintfd ( "!!!!! begin syscall readdir(/)\n" );
  vfs_syscall.readdir ( "/" );
  kprintfd ( "!!!!! end syscall readdir(/)\n" );

  kprintfd ( "!!!!! begin syscall readdir(chen)\n" );
  vfs_syscall.readdir ( "chen" );
  kprintfd ( "!!!!! end syscall readdir(chen)\n" );

  kprintfd ( "!!!!! begin syscall readdir(../../../chen/../chen/./chen)\n" );
  vfs_syscall.readdir ( "../../../chen/../chen/./chen" );
  kprintfd ( "!!!!! end syscall readdir(../../../chen/../chen/./chen)\n" );

  kprintfd ( "!!!!! begin syscall readdir(./chen/chen/chen)\n" );
  vfs_syscall.readdir ( "./chen/chen/chen" );
  kprintfd ( "!!!!! end syscall readdir(./chen/chen/chen)\n" );

  kprintfd ( "!!!!! begin syscall readdir(./chen/chen/./chen)\n" );
  vfs_syscall.readdir ( "./chen/chen/chen/./chen" );
  kprintfd ( "!!!!! end syscall readdir(./chen/chen/./chen)\n" );

  kprintfd ( "!!!!! begin syscall readdir(./chen/chen/chen/chen/chen)\n" );
  vfs_syscall.readdir ( "./chen/chen/chen/chen/chen" );
  kprintfd ( "!!!!! end syscall readdir(./chen/chen/chen/chen/chen)\n" );

  kprintfd ( "!!!!! begin syscall readdir(/chen/chen/qiang)\n" );
  vfs_syscall.readdir ( "/chen/chen/qiang" );
  kprintfd ( "!!!!! end syscall readdir(/chen/chen/qiang)\n" );
}

//----------------------------------------------------------------------
void testSyscallChdir()
{
  kprintfd ( "!!!!! begin syscall chdir(/chen)\n" );
  vfs_syscall.chdir ( "/chen" );
  vfs_syscall.readdir ( "." );
  kprintfd ( "!!!!! end syscall chdir(/chen)\n" );

  kprintfd ( "!!!!! begin syscall chdir(chen)\n" );
  vfs_syscall.chdir ( "chen" );
  vfs_syscall.readdir ( "." );
  kprintfd ( "!!!!! end syscall chdir(chen)\n" );

  kprintfd ( "!!!!! begin syscall chdir(../../../chen/../chen/./chen)\n" );
  vfs_syscall.chdir ( "../../../chen/../chen/./chen" );
  vfs_syscall.readdir ( "." );
  kprintfd ( "!!!!! end syscall chdir(../../../chen/../chen/./chen)\n" );

  kprintfd ( "!!!!! begin syscall chdir(./chen/chen/chen)\n" );
  vfs_syscall.chdir ( "./chen/chen/chen" );
  vfs_syscall.readdir ( "." );
  kprintfd ( "!!!!! end syscall chdir(./chen/chen/chen)\n" );

  kprintfd ( "!!!!! begin syscall chdir(./chen/chen/./chen)\n" );
  vfs_syscall.chdir ( "./chen/chen/chen/./chen" );
  vfs_syscall.readdir ( "." );
  kprintfd ( "!!!!! end syscall chdir(./chen/chen/./chen)\n" );

  kprintfd ( "!!!!! begin syscall chdir(./chen/chen/chen/chen/chen)\n" );
  vfs_syscall.chdir ( "./chen/chen/chen/chen/chen" );
  vfs_syscall.readdir ( "." );
  kprintfd ( "!!!!! end syscall chdir(./chen/chen/chen/chen/chen)\n" );

  kprintfd ( "!!!!! begin syscall chdir(/chen/chen/qiang)\n" );
  vfs_syscall.chdir ( "/chen/chen/qiang" );
  vfs_syscall.readdir ( "." );
  kprintfd ( "!!!!! end syscall chdir(/chen/chen/qiang)\n" );
}

//----------------------------------------------------------------------
void testSyscallRmdir()
{
  kprintfd ( "***** begin syscall rmdir(chen)\n" );
  vfs_syscall.rmdir ( "chen" );
  kprintfd ( "***** end syscall rmdir(chen)\n" );

  kprintfd ( "***** begin syscall rmdir(chen)\n" );
  vfs_syscall.rmdir ( "/chen/qiang" );
  kprintfd ( "***** end syscall rmdir(chen)\n" );

  kprintfd ( "***** begin syscall rmdir(chen)\n" );
  vfs_syscall.rmdir ( "/chen/2006" );
  kprintfd ( "***** end syscall rmdir(chen)\n" );

  kprintfd ( "***** begin syscall rmdir(chen)\n" );
  vfs_syscall.rmdir ( "/chen/2005" );
  kprintfd ( "***** end syscall rmdir(chen)\n" );

  kprintfd ( "***** begin syscall rmdir(chen)\n" );
  vfs_syscall.rmdir ( "/chen/2007" );
  kprintfd ( "***** end syscall rmdir(chen)\n" );

  kprintfd ( "***** begin syscall rmdir(chen/chen)\n" );
  vfs_syscall.rmdir ( "chen/chen" );
  kprintfd ( "***** end syscall rmdir(chen/chen)\n" );

  kprintfd ( "***** begin syscall rmdir(chen/chen)\n" );
  vfs_syscall.rmdir ( "chen/chen/chen" );
  kprintfd ( "***** end syscall rmdir(chen/chen)\n" );

  kprintfd ( "***** begin syscall rmdir(chen/chen)\n" );
  vfs_syscall.rmdir ( "./.././chen/chen/chen/chen" );
  kprintfd ( "***** end syscall rmdir(chen/chen)\n" );

  kprintfd ( "***** begin syscall rmdir(chen/chen)\n" );
  vfs_syscall.rmdir ( "chen/chen/.././chen/chen/chen/.././chen/chen" );
  kprintfd ( "***** end syscall rmdir(chen/chen)\n" );

  kprintfd ( "***** begin syscall rmdir(chen/chen)\n" );
  vfs_syscall.rmdir ( "chen/chen/chen/chen/chen" );
  kprintfd ( "***** end syscall rmdir(chen/chen)\n" );
}

//----------------------------------------------------------------------
void testSyscallRmdirExtern()
{
  vfs_syscall.mkdir ( "chen", 0 );
  vfs_syscall.chdir ( "chen" );
  vfs_syscall.mkdir ( "chen", 0 );
  vfs_syscall.chdir ( "chen" );
  vfs_syscall.mkdir ( "chen", 0 );
  vfs_syscall.chdir ( "chen" );
  vfs_syscall.mkdir ( "chen", 0 );
  vfs_syscall.chdir ( "chen" );
  vfs_syscall.mkdir ( "chen", 0 );
  vfs_syscall.chdir ( "chen" );
  vfs_syscall.chdir ( ".." );
  vfs_syscall.rmdir ( "chen" );
  vfs_syscall.chdir ( ".." );
  vfs_syscall.rmdir ( "chen" );
  vfs_syscall.chdir ( ".." );
  vfs_syscall.rmdir ( "chen" );
  vfs_syscall.chdir ( ".." );
  vfs_syscall.rmdir ( "chen" );
  vfs_syscall.chdir ( ".." );
  vfs_syscall.rmdir ( "chen" );
}

//----------------------------------------------------------------------
void testMini()
{
  vfs_syscall.mkdir ( "chen",0 );
  vfs_syscall.mkdir ( "qiang",0 );
  vfs_syscall.readdir ( "." );
  vfs_syscall.chdir ( "qiang" );
  vfs_syscall.readdir ( ".." );
}

//----------------------------------------------------------------------
void testMount()
{
  RamFSType *ramfs = new RamFSType();
  vfs.registerFileSystem ( ramfs );

  PseudoFSType *pfs = new PseudoFSType();
  vfs.registerFileSystem ( pfs );

  vfs.root_mount ( "ramfs", 0 );
}

//----------------------------------------------------------------------
void testUmount()
{}

//----------------------------------------------------------------------
void testPathWalker()
{
  kprintfd ( "***** start of mdkir(/chen)\n" );
  vfs_syscall.mkdir ( "/chen", 0 );
  kprintfd ( "***** end of mkdir(/chen)\n" );

  kprintfd ( "***** start of mkdir(test)\n" );
  vfs_syscall.mkdir ( "test", 0 );
  kprintfd ( "***** end of mkdir(test)\n" );

  kprintfd ( "***** start of chdir(chen)\n" );
  vfs_syscall.chdir ( "chen" );
  kprintfd ( "***** end of chdir(chen)\n" );

  kprintfd ( "***** start of mkdir(./hugo)\n" );
  vfs_syscall.mkdir ( "./hugo", 0 );
  kprintfd ( "***** end of mkdir(./hugo)\n" );

  kprintfd ( "***** start of mkdir(test)\n" );
  vfs_syscall.mkdir ( "./../chen/.././SSS", 0 );
  kprintfd ( "***** end of mkdir(test)\n" );

  kprintfd ( "***** start of mkdir(../hugo)\n" );
  vfs_syscall.mkdir ( "../hugo", 0 );
  kprintfd ( "***** end of mkdir(../hugo)\n" );


  kprintfd ( "***** start of readdir(/)\n" );
  vfs_syscall.readdir ( "/" );
  kprintfd ( "***** end of readdir(/)\n" );

  kprintfd ( "***** start of readdir(/)\n" );
  vfs_syscall.readdir ( "/chen" );
  kprintfd ( "***** end of readdir(/)\n" );
}

//----------------------------------------------------------------------
void testVfsSyscall()
{
  kprintfd ( "***** start of mdkir(/chen)\n" );
  vfs_syscall.mkdir ( "/chen", 0 );
  kprintfd ( "***** end of mkdir(/chen)\n" );

  kprintfd ( "***** start of mkdir(/chen/qiang)\n" );
  vfs_syscall.mkdir ( "/chen/qiang", 0 );
  kprintfd ( "***** end of mkdir(/chen/qiang)\n" );

  kprintfd ( "***** start of mkdir(/chen/qiang/always)\n" );
  vfs_syscall.mkdir ( "/chen/qiang/always", 0 );
  kprintfd ( "***** end of mkdir(/chen/qiang/always)\n" );

  kprintfd ( "***** start of mkdir(/chen/2005)\n" );
  vfs_syscall.mkdir ( "/chen/2005", 0 );
  kprintfd ( "***** end of mkdir(/chen/2005)\n" );

  kprintfd ( "***** start of readdir(/chen)\n" );
  vfs_syscall.readdir ( "/chen/" );
  kprintfd ( "***** end of readdir(/chen)\n" );

  kprintfd ( "***** start of chdir(/chen/qiang)\n" );
  vfs_syscall.chdir ( "/chen/qiang/" );
  kprintfd ( "***** end of chdir(/chen/qiang)\n" );

  kprintfd ( "***** start of mkdir(/chen/qiang/always/dead)\n" );
  vfs_syscall.mkdir ( "/chen/qiang/always/dead", 0 );
  kprintfd ( "***** end of mkdir(/chen/qiang/always/dead)\n" );

  kprintfd ( "***** start of rmdir(/chen/qiang/always/dead)\n" );
  vfs_syscall.rmdir ( "/chen/qiang/always/dead" );
  kprintfd ( "***** end of rmdir(/chen/qiang/always/dead)\n" );
}

//----------------------------------------------------------------------
#include "fs/ramfs/RamFSInode.h"
#include "fs/ramfs/RamFSSuperblock.h"
#include "fs/Dentry.h"

void testReadWriteInode()
{
  FileSystemInfo *fs_info = currentThread->getFSInfo();
  Dentry *dentry = fs_info->getRoot();
  Inode *inode = dentry->getInode();
  Superblock *sb = inode->getSuperblock();

  Inode *test_inode = ( Inode* ) ( new RamFSInode ( sb, I_FILE ) );
  test_inode->writeData ( 0, 13, "do write data" );

  char test_array[20];
  test_inode->readData ( 0, 13, test_array );
  test_array[13] = '\0';

  kprintfd ( "test_array = %s\n", test_array );
}

