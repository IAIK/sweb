cmake_minimum_required(VERSION 3.1.0)

file(GLOB util_mkminixfs_SOURCES *.cpp
                                   ../../common/source/util/Bitmap.cpp
                                   ../../common/source/fs/Dentry.cpp
                                   ../../common/source/fs/FileDescriptor.cpp
                                   ../../common/source/fs/FileSystemInfo.cpp
                                   ../../common/source/fs/Superblock.cpp
                                   ../../common/source/fs/File.cpp
                                   ../../common/source/fs/PathWalker.cpp
                                   ../../common/source/fs/VfsMount.cpp
                                   ../../common/source/fs/VfsSyscall.cpp
                                   ../../common/source/fs/minixfs/MinixFSFile.cpp
                                   ../../common/source/fs/minixfs/MinixFSInode.cpp
                                   ../../common/source/fs/minixfs/MinixFSSuperblock.cpp
                                   ../../common/source/fs/minixfs/MinixFSZone.cpp
                                   ../../common/source/fs/minixfs/MinixStorageManager.cpp
                                   ../../common/source/fs/minixfs/StorageManager.cpp)

add_executable(mkminixfs ${util_mkminixfs_SOURCES})

target_include_directories(mkminixfs
  PRIVATE
    .
    ../exe2minixfs/
    ../exe2minixfs/ustl
    ../../common/include/util
    ../../common/include/fs
    ../../common/include/fs/minixfs
)

target_compile_definitions(mkminixfs PRIVATE EXE2MINIXFS=1)
