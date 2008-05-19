include_directories(../../include/kernel)

FILE(GLOB common_kernel_SOURCES ${SOURCE_WILDCARDS})

ADD_LIBRARY(common_kernel ${common_kernel_SOURCES})

set(ENV{LIBRARY_NAMES} "$ENV{LIBRARY_NAMES};common_kernel")