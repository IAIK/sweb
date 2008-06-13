project(sweb_libc)


set(CMAKE_C_FLAGS "-Wall -g -m32 -fno-builtin -nostdinc -fno-stack-protector -fno-common")

include_directories(
    include
    include/sys
)


#Assembly File processing
set(ASM_OBJECTS)

foreach(src start syscalls)
   set(ASM_SOURCE ../../arch/${ARCH}/userspace/${src}.s)
   set(ASM_OBJECT ${CMAKE_CURRENT_BINARY_DIR}/${src}.o)
   set(ASM_OBJECTS ${ASM_OBJECTS} ${ASM_OBJECT})
   add_custom_command(
     OUTPUT ${ASM_OBJECT}
     COMMAND ${NASM_EXECUTABLE}
     ARGS    -f elf -o ${ASM_OBJECT} ${ASM_SOURCE}
     DEPENDS ${ASM_SOURCE}
     )
ENDFOREACH(src)



set(LIBRARY_OUTPUT_PATH  "${LIBRARY_OUTPUT_PATH}/userspace")
FILE(GLOB userspace_libc_SOURCES src/*.c)

ADD_LIBRARY(userspace_libc ${userspace_libc_SOURCES} ${ASM_OBJECTS})
    