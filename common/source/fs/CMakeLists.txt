include_directories(../../include/fs)

add_project_library(common_fs)

add_subdirectory(device)
add_subdirectory(inodes)
add_subdirectory(minix)
add_subdirectory(unixfs)

