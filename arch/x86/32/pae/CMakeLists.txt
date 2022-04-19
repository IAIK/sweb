target_include_directories(kernel PUBLIC
    ../../../common/include
    ../../common/include
    ../common/include
    include
    common/include
)

add_subdirectory(source)
