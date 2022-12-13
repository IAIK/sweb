add_project_library(common_drivers)

target_include_directories(kernel PUBLIC
    ../../include/drivers)

add_subdirectory(platform)
