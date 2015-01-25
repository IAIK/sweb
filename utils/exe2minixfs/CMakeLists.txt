set(CMAKE_C_FLAGS "-std=gnu11 -g")
set(CMAKE_CXX_FLAGS "-std=gnu++11 -g")

file(GLOB util_exe2minixfs_SOURCES *.cpp)

add_executable(exe2minixfs ${util_exe2minixfs_SOURCES})

