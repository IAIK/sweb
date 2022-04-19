target_include_directories(kernel PUBLIC
    x86/common/include
    ../common/include
    include
    common/include
)

add_subdirectory(source)
add_subdirectory(common)
