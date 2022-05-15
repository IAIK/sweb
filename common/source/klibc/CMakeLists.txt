add_library(kernel_libc)

target_include_directories(kernel_libc
    PUBLIC
    ../../include/klibc
)


file(GLOB source_files CONFIGURE_DEPENDS ${SOURCE_WILDCARDS})

target_sources(kernel_libc PRIVATE
    ${source_files})
