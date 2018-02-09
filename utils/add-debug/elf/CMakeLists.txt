# Attention: This CMakeLists.txt implements the bare minimum of what is
#            required by SWEB.

cmake_minimum_required(VERSION 3.0.2)

set(CMAKE_CXX_STANDARD 11)

add_custom_command(
    OUTPUT to_string.cc
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}"
    COMMAND make to_string.cc
    DEPENDS enum-print.py data.hh to_hex.hh Makefile
)

add_library(elf++ STATIC elf.cc mmap_loader.cc to_string.cc)
