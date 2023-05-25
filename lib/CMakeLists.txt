cmake_minimum_required(VERSION 3.13)

# https://eastl.docsforge.com/master/cmake/project-integration/

add_subdirectory(EASTL)

target_link_libraries(EASTL kernel_options)
target_link_libraries(EASTL kernel_libc)
target_include_directories(EASTL
    PRIVATE
    "$<TARGET_PROPERTY:kernel,INTERFACE_INCLUDE_DIRECTORIES>"
)
target_sources(EASTL PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/EASTL/doc/EASTL.natvis")

target_compile_options(EASTL PRIVATE -Wno-unused-variable)

target_include_directories(EASTL PUBLIC EASTL_userconfig)
target_compile_definitions(EASTL PUBLIC
    EASTL_USER_CONFIG_HEADER="eastl_userconfig.h"
    EA_PLATFORM_SWEB=1 # EABase dependency does not check userconfig header. Need to define this globally.
)

target_link_libraries(kernel PUBLIC EASTL)
