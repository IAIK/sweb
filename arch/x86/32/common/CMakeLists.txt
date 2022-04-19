target_include_directories(kernel PUBLIC
    ../../../${ARCH}/include
    include
    ../include
)

add_subdirectory(source)
add_subdirectory(userspace)
