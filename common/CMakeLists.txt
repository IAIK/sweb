target_include_directories(kernel PUBLIC
    include
    ../arch/common/include
    ../arch/${ARCH}/include
    ../arch/${ARCH}/common/include
    ../arch/${ARCH}/../common/include
    ../arch/${ARCH}/../../common/include
    include/console
    include/kernel
    include/drivers
    include/fs
    include/fs/devicefs
    include/fs/devicefs/devices
    include/fs/minixfs
    include/fs/pseudofs
    include/fs/ramfs
    include/ipc
    include/mm
    include/mm/allocators
    include/util
)

add_subdirectory(source)
