target_include_directories(kernel PUBLIC
    common/include
    common/include/usb
    ../${ARCH}/include
)

add_subdirectory(common/source)
add_subdirectory(common/source/usb)
add_subdirectory(common/userspace)
