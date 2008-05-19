#Assembly File processing
set(ASM_OBJECTS)

foreach(src boot arch_interrupts arch_switch)
   set(ASM_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/${src}.s)
   set(ASM_OBJECT ${CMAKE_CURRENT_BINARY_DIR}/${src}.o)
   set(ASM_OBJECTS ${ASM_OBJECTS} ${ASM_OBJECT})
   add_custom_command(
     OUTPUT ${ASM_OBJECT}
     COMMAND ${NASM_EXECUTABLE}
     ARGS    -f elf -o ${ASM_OBJECT} ${ASM_SOURCE}
     DEPENDS ${ASM_SOURCE}
     )
ENDFOREACH(src)


file(GLOB arch_x86_SOURCES ${SOURCE_WILDCARDS})

add_library(arch_x86 ${arch_x86_SOURCES} ${ASM_OBJECTS})
set(ENV{LIBRARY_NAMES} "$ENV{LIBRARY_NAMES};arch_x86")
