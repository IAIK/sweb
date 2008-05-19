include_directories(../../include/fs/pseudofs)

FILE(GLOB common_fs_pseudofs_SOURCES ${SOURCE_WILDCARDS})

ADD_LIBRARY(common_fs_pseudofs ${common_fs_pseudofs_SOURCES})

set(ENV{LIBRARY_NAMES} "$ENV{LIBRARY_NAMES};common_fs_pseudofs")