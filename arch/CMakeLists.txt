
include_directories(common/include
    ../common/include
    ../common/include/cache
    ../common/include/console
    ../common/include/fs
    ../common/include/fs/device
    ../common/include/fs/inodes
    ../common/include/fs/minix
    ../common/include/fs/unixfs
    ../common/include/kernel
    ../common/include/mm
    ../common/include/ustl
    ../common/include/util
)

add_subdirectory(common/source)

include(${ARCH}/CMakeLists.subfolders)

