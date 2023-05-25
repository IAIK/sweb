target_include_directories(kernel PUBLIC
    common/include
    ../${ARCH}/include
    ../${ARCH}/common/include
    ../${ARCH}/../common/include
    ../${ARCH}/../../common/include
)

add_subdirectory(common/source)
