set(CMAKE_C_FLAGS "-std=gnu11")
set(CMAKE_CXX_FLAGS "-std=gnu++11")

file(GLOB util_exe2minixfs_SOURCES *.cpp)

add_executable(exe2minixfs ${util_exe2minixfs_SOURCES})

