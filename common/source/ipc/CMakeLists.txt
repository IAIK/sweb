include_directories(../../include/ipc)

FILE(GLOB common_ipc_SOURCES ${SOURCE_WILDCARDS})

ADD_LIBRARY(common_ipc ${common_ipc_SOURCES})

set(ENV{LIBRARY_NAMES} "$ENV{LIBRARY_NAMES};common_ipc")