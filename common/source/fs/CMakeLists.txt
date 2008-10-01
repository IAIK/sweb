include_directories(../../include/fs)

add_project_library(common_fs)

add_subdirectory(devicefs)
add_subdirectory(minixfs)
add_subdirectory(ramfs)
add_subdirectory(pseudofs)
add_subdirectory(tests)