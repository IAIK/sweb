include_directories(../../include/fs)

FILE(GLOB common_fs_SOURCES ${SOURCE_WILDCARDS})

ADD_LIBRARY(common_fs ${common_fs_SOURCES})

set(ENV{LIBRARY_NAMES} "$ENV{LIBRARY_NAMES};common_fs")

add_subdirectory(devicefs)
add_subdirectory(minixfs)
add_subdirectory(ramfs)
add_subdirectory(pseudofs)
add_subdirectory(tests)