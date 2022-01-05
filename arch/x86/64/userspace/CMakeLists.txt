cmake_minimum_required(VERSION 3.11)

FILE(GLOB userspace_libc_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/*.c)

target_sources(userspace_libc
  PRIVATE
    ${userspace_libc_SOURCES}
    )
