include_directories(../../include/console)

FILE(GLOB common_console_SOURCES ${SOURCE_WILDCARDS})

ADD_LIBRARY(common_console ${common_console_SOURCES})

set(ENV{LIBRARY_NAMES} "$ENV{LIBRARY_NAMES};common_console")
