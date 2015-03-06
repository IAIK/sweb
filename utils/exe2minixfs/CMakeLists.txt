include_directories(
  .
  ustl
  ../../common/include/util
  ../../common/include/fs
  ../../common/include/fs/minixfs
)

set(CMAKE_C_FLAGS "-DEXE2MINIXFS   -std=gnu11   -O0 -g -Wall -Wextra -fno-builtin -fno-rtti -fno-exceptions")
set(CMAKE_CXX_FLAGS "-DEXE2MINIXFS -std=gnu++11 -O0 -g -Wall -Wextra -fno-builtin -fno-rtti -fno-exceptions")

file(GLOB util_exe2minixfs_SOURCES *.cpp
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

add_executable(exe2minixfs ${util_exe2minixfs_SOURCES})

