project(sweb_libc)
include(../../arch/${ARCH}/CMakeLists.userspace)

include_directories(
    include
    include/sys
)


set(LIBRARY_OUTPUT_PATH  "${LIBRARY_OUTPUT_PATH}/userspace")
FILE(GLOB userspace_libc_SOURCES src/*.c ${CMAKE_SOURCE_DIR}/arch/${ARCH}/userspace/*.c ${CMAKE_SOURCE_DIR}/arch/${ARCH}/../common/userspace/*.c ${CMAKE_SOURCE_DIR}/arch/${ARCH}/common/userspace/*.c)

ADD_LIBRARY(userspace_libc ${userspace_libc_SOURCES})

