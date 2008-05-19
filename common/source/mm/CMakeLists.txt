include_directories(../../include/mm)

FILE(GLOB common_mm_SOURCES ${SOURCE_WILDCARDS})

ADD_LIBRARY(common_mm ${common_mm_SOURCES})

set(ENV{LIBRARY_NAMES} "$ENV{LIBRARY_NAMES};common_mm")