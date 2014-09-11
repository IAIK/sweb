project(sweb_libc)
include(../../arch/${ARCH}/CMakeLists.userspace)

include_directories(
    include
    include/sys
)


#Assembly File processing
set(ASM_OBJECTS)

foreach(src syscalls)
   set(ASM_SOURCE ${CMAKE_SOURCE_DIR}/arch/${ARCH}/userspace/${src}.s)
   if(NOT EXISTS ${ASM_SOURCE})
     set(ASM_SOURCE ${CMAKE_SOURCE_DIR}/arch/${ARCH}/../common/userspace/${src}.s)
   endif()
   set(ASM_OBJECT ${CMAKE_CURRENT_BINARY_DIR}/${src}.o)
   set(ASM_OBJECTS ${ASM_OBJECTS} ${ASM_OBJECT})
   add_custom_command(
     OUTPUT ${ASM_OBJECT}
     COMMAND ${ASM_EXECUTABLE}
     ARGS    ${ASM_FORMAT} -o ${ASM_OBJECT} ${ASM_SOURCE}
     DEPENDS ${ASM_SOURCE}
     )
ENDFOREACH(src)



set(LIBRARY_OUTPUT_PATH  "${LIBRARY_OUTPUT_PATH}/userspace")
FILE(GLOB userspace_libc_SOURCES src/*.c)

ADD_LIBRARY(userspace_libc ${userspace_libc_SOURCES} ${ASM_OBJECTS})

