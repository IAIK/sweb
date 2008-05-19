project(exe2pseudofs)

set(CMAKE_CXX_FLAGS "-fno-builtin -m32")

file(GLOB util_exe2pseudofs_SOURCES *.cpp)

include_directories(.)

add_executable(exe2pseudofs ${util_exe2pseudofs_SOURCES})
