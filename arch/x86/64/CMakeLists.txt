target_include_directories(kernel PUBLIC
    ../../common/include
    ../common/include
    include
)

add_subdirectory(source)
add_subdirectory(userspace)
