add_project_library(common_fs)

target_include_directories(kernel PUBLIC
    ../../include/fs)

add_subdirectory(devicefs)
add_subdirectory(minixfs)
add_subdirectory(ramfs)
