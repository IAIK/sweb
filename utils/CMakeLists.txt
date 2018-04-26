# do not cross compile host tools
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_C_COMPILER gcc)
set(CMAKE_AR ar)
set(CMAKE_RANLIB ranlib)

# Ugly hack for preventing kernel CFLAGS from preventing utils build
# TODO: These should not be set for utils/ in the first place
set(UTILS_CFLAGS "-O0 -g -Wall -Wextra -Werror -fno-rtti -fno-exceptions")

set(CMAKE_C_FLAGS   "-std=gnu11   ${UTILS_CFLAGS}")
set(CMAKE_CXX_FLAGS "-std=gnu++11 ${UTILS_CFLAGS}")

add_subdirectory(exe2minixfs)
add_subdirectory(add-debug)
add_subdirectory(images)
