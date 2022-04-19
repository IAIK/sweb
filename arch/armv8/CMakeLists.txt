target_include_directories(kernel PUBLIC
    common/include
    ../${ARCH}/include
)

add_subdirectory(common/source)
add_subdirectory(common/userspace)
