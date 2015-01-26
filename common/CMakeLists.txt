include_directories(
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
    include/fs/minixfs
    include/fs/pseudofs
    include/fs/ramfs
    include/ipc
    include/mm
    include/util
    include/ustl
)

if(${ARCH} STREQUAL "xen")
    include_directories(
    ../arch/xen/x86/include
    )
endif(${ARCH} STREQUAL "xen")

add_subdirectory(source)
