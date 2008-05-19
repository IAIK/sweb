project(userspace_tests)

include_directories(../libc/include)

FILE(GLOB userspace_tests_SOURCES ${SOURCE_WILDCARDS})

#ADD_LIBRARY(common_console ${userspace_tests_SOURCES})