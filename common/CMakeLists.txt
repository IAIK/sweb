include_directories(
    include
    ../arch/common/include
    ../arch/${ARCH}/include    
    include/console
    include/kernel
    include/drivers
    include/fs
#    include/fs/pseudofs
#    include/fs/minixfs
#    include/fs/ramfs
#    include/fs/devicefs
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
