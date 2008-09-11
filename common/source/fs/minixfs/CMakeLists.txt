include_directories(../../../include/fs/minixfs)

FILE(GLOB common_fs_minixfs_SOURCES ${SOURCE_WILDCARDS})

ADD_LIBRARY(common_fs_minixfs ${common_fs_minixfs_SOURCES})

set(ENV{LIBRARY_NAMES} "$ENV{LIBRARY_NAMES};common_fs_minixfs")