include_directories(
  ./
  ../../common/include/util
  ../../common/include/fs
)

set(CMAKE_C_FLAGS "-DEXE2MINIXFS -std=gnu11 -g")
set(CMAKE_CXX_FLAGS "-DEXE2MINIXFS -std=gnu++11 -g")

file(GLOB util_exe2minixfs_SOURCES *.cpp
                                   ../../common/source/util/Bitmap.cpp
                                   #../../common/source/fs/File.cpp
                                   ../../common/source/fs/Dentry.cpp
                                   ../../common/source/fs/minixfs/MinixFSFile.cpp
                                   ../../common/source/fs/minixfs/MinixFSInode.cpp
                                   ../../common/source/fs/minixfs/MinixFSZone.cpp
                                   ../../common/source/fs/minixfs/MinixStorageManager.cpp
                                   ../../common/source/fs/minixfs/StorageManager.cpp)

add_executable(exe2minixfs ${util_exe2minixfs_SOURCES})

