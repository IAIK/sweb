include_directories(../../include/drivers)

FILE(GLOB common_drivers_SOURCES ${SOURCE_WILDCARDS})

ADD_LIBRARY(common_drivers ${common_drivers_SOURCES})

set(ENV{LIBRARY_NAMES} "$ENV{LIBRARY_NAMES};common_drivers")