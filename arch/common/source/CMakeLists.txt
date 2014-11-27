include_directories(
    ../include
    ../../${ARCH}/include
    ../../${ARCH}/common/include
    ../../${ARCH}/../common/include
    ../../${ARCH}/../../common/include
)

add_project_library(arch_common)
