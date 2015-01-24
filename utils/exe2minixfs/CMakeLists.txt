set(CMAKE_C_FLAGS)
set(CMAKE_CXX_FLAGS)

file(GLOB util_exe2minixfs_SOURCES *.cpp)

add_executable(exe2minixfs ${util_exe2minixfs_SOURCES})

