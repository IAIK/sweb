cmake_minimum_required(VERSION 3.11)

project(userspace_tests)
include(../../arch/${ARCH}/CMakeLists.userspace)

# Put userspace libraries and executables in seperated paths
set(LIBRARY_OUTPUT_PATH  "${LIBRARY_OUTPUT_PATH}/userspace")
set(EXECUTABLE_OUTPUT_PATH "${EXECUTABLE_OUTPUT_PATH}/userspace")

# Reset userspace program names
set($ENV{USERSPACE_NAMES} "")

target_link_options(userspace_options INTERFACE
    -Wl,-Ttext=0x8000000 -Wl,--build-id=none -Wl,--whole-archive)


# Create own executable for every .c file and link with libc
file(GLOB userspace_tests_SOURCES CONFIGURE_DEPENDS ${SOURCE_WILDCARDS})
foreach(curFile ${userspace_tests_SOURCES})
  get_filename_component(curName ${curFile} NAME_WE)

  add_executable(${curName}.sweb ${curFile})

  target_link_libraries(${curName}.sweb PUBLIC userspace_libc)
  target_link_libraries(${curName}.sweb PUBLIC userspace_options)

  ADD_DEBUG_INFO(${curName}.sweb)

  # Remember the userspace program names for dependency checking in the root CMakeLists
  set(ENV{USERSPACE_NAMES} "$ENV{USERSPACE_NAMES};${curName}.sweb")
  set(ENV{USERSPACE_NAMES_EXE2MINIX} "$ENV{USERSPACE_NAMES_EXE2MINIX};${EXECUTABLE_OUTPUT_PATH}/${curName}.sweb;${curName}.sweb")
endforeach(curFile)


# All files in a directory get linked together
file(GLOB userspace_tests_PROJECTS CONFIGURE_DEPENDS */)
foreach(curProject ${userspace_tests_PROJECTS})
  file(GLOB curProject_SOURCES CONFIGURE_DEPENDS ${curProject}/*.c)

  if(curProject_SOURCES)
    get_filename_component(exename ${curProject} NAME)

    add_executable(${exename}.sweb ${curProject_SOURCES})

    target_link_libraries(${exename}.sweb PUBLIC userspace_libc)
    target_link_libraries(${curName}.sweb PUBLIC userspace_options)

    ADD_DEBUG_INFO(${curName}.sweb)

    set(ENV{USERSPACE_NAMES} "$ENV{USERSPACE_NAMES};${exename}.sweb")
    set(ENV{USERSPACE_NAMES_EXE2MINIX} "$ENV{USERSPACE_NAMES_EXE2MINIX};${EXECUTABLE_OUTPUT_PATH}/${exename}.sweb;${exename}.sweb")
  endif(curProject_SOURCES)
endforeach(curProject)
