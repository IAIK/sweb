include_directories(
  .
  ../../common/include/util
)

set(CMAKE_C_FLAGS "-DEXE2MINIXFS -std=gnu11 -g")
set(CMAKE_CXX_FLAGS "-DEXE2MINIXFS -std=gnu++11 -g")

file(GLOB util_exe2minixfs_SOURCES *.cpp
                                   ../../common/source/util/Bitmap.cpp
                                   ../../common/source/fs/minixfs/MinixFSFile.cpp
                                   ../../common/source/fs/minixfs/MinixFSInode.cpp)

add_executable(exe2minixfs ${util_exe2minixfs_SOURCES})

