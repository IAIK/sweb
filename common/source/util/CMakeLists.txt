include_directories(../../include/util)

FILE(GLOB common_util_SOURCES ${SOURCE_WILDCARDS})

ADD_LIBRARY(common_util ${common_util_SOURCES})

set(ENV{LIBRARY_NAMES} "$ENV{LIBRARY_NAMES};common_util")