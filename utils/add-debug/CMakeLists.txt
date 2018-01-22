cmake_minimum_required(VERSION 3.0.2)

set(CMAKE_CXX_STANDARD 11)

# libelfin uses exceptions and has a few warnings
set(LIBELFIN_CFLAGS "-fexceptions -frtti -Wno-unused-parameter -Wno-missing-field-initializers")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${LIBELFIN_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LIBELFIN_CFLAGS}")

add_subdirectory(elf)
add_subdirectory(dwarf)

add_executable(add-dbg add-dbg.cc)
target_include_directories(add-dbg PRIVATE elf dwarf)
target_link_libraries(add-dbg elf++ dwarf++)
