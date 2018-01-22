# Attention: This CMakeLists.txt implements the bare minimum of what is
#            required by SWEB.

cmake_minimum_required(VERSION 3.0.2)

set(CMAKE_CXX_STANDARD 11)


add_library(elf++ STATIC elf.cc mmap_loader.cc to_string.cc)

add_custom_command(
    TARGET elf++
    PRE_BUILD
    COMMAND make to_string.cc
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
