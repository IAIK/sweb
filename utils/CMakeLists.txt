# do not cross compile host tools
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_C_COMPILER gcc)

add_subdirectory(exe2minixfs)

