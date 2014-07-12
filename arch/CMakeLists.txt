
include_directories(common/include
    ../common/include
    ../common/include/console
    ../common/include/kernel
    ../common/include/drivers
    ../common/include/fs
    ../common/include/fs/pseudofs
    ../common/include/fs/minixfs
    ../common/include/fs/ramfs
    ../common/include/fs/devicefs
    ../common/include/ipc
    ../common/include/mm
    ../common/include/util
)

add_subdirectory(common/source)

include(${ARCH}/CMakeLists.subfolders)

