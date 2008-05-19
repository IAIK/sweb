FILE(GLOB common_fs_tests_SOURCES ${SOURCE_WILDCARDS})

ADD_LIBRARY(common_fs_tests ${common_fs_tests_SOURCES})

set(ENV{LIBRARY_NAMES} "$ENV{LIBRARY_NAMES};common_fs_tests")