target_include_directories(kernel PUBLIC
    ../common/include/usb
    ../common/include
    include
)

add_subdirectory(source)
