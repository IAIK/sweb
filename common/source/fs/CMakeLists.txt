file(GLOB_RECURSE Foo_SOURCES "device/*.cpp" "inodes/*.cpp" "unixfs/*.cpp" "minix/*.cpp" "tests/*.cpp")
file(GLOB_RECURSE Foo_HEADERS "../../include/fs/*.h")

#message(STATUS ${Foo_SOURCES})
#message(STATUS ${Foo_HEADERS})

set (Foo_INCLUDE_DIRS "")
foreach (_headerFile ${Foo_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND Foo_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES Foo_INCLUDE_DIRS)

include_directories(${Foo_INCLUDE_DIRS})
add_project_library_sub_directories(common_fs ${Foo_SOURCES})

#----------------------------------------------------------------------
# old CMakeLists.txt
#----------------------------------------------------------------------

#include_directories(../../include/fs)
#add_project_library(common_fs)
