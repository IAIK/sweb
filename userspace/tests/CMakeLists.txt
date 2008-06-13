project(userspace_tests)

# Put userspace libraries and executables in seperated paths
set(LIBRARY_OUTPUT_PATH  "${LIBRARY_OUTPUT_PATH}/userspace")
set(EXECUTABLE_OUTPUT_PATH "${EXECUTABLE_OUTPUT_PATH}/userspace")

set(CMAKE_C_FLAGS "-O0 -m32 -static -nostdinc -fno-builtin -nostdlib -fno-stack-protector")

# Reset userspace program names
set($ENV{USERSPACE_NAMES} "")

link_directories(${LIBRARY_OUTPUT_PATH})
include_directories(../libc/include)

file(GLOB userspace_tests_SOURCES ${SOURCE_WILDCARDS})

# Create own executable for every .c file and link with libc
foreach(curFile ${userspace_tests_SOURCES})
	get_filename_component(curPath ${curFile} PATH)
	get_filename_component(curName ${curFile} NAME_WE)
	
	add_executable(${curName}.sweb ${curFile})
	target_link_libraries(${curName}.sweb "-Wl,-whole-archive" userspace_libc)

	#Remember the userspace program names for dependency checking in the root CMakeLists
	set(ENV{USERSPACE_NAMES} "$ENV{USERSPACE_NAMES};${curName}.sweb")	
	set(ENV{USERSPACE_NAMES_EXE2MINIX} "$ENV{USERSPACE_NAMES_EXE2MINIX};${EXECUTABLE_OUTPUT_PATH}/${curName}.sweb;${curName}.sweb")	
endforeach(curFile)


