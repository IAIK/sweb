include_directories(../../../include/fs/ramfs)

FILE(GLOB common_fs_ramfs_SOURCES ${SOURCE_WILDCARDS})

ADD_LIBRARY(common_fs_ramfs ${common_fs_ramfs_SOURCES})

set(ENV{LIBRARY_NAMES} "$ENV{LIBRARY_NAMES};common_fs_ramfs")