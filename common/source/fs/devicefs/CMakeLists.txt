include_directories(../../include/fs/devicefs)

FILE(GLOB common_fs_devicefs_SOURCES ${SOURCE_WILDCARDS})

ADD_LIBRARY(common_fs_devicefs ${common_fs_devicefs_SOURCES})

set(ENV{LIBRARY_NAMES} "$ENV{LIBRARY_NAMES};common_fs_devicefs")