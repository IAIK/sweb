cmake_minimum_required(VERSION 3.13)

cmake_policy(SET CMP0079 NEW)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

# Include custom CMake modules from the cmake/ subdirectory
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

project(sweb
    LANGUAGES C CXX ASM)

add_executable(kernel)
set_target_properties(kernel PROPERTIES OUTPUT_NAME kernel.x)

add_library(kernel_options INTERFACE) # https://cmake.org/cmake/help/latest/manual/cmake-buildsystem.7.html#interface-libraries
target_link_libraries(kernel PUBLIC kernel_options)

set(HDD_IMAGE_RAW "SWEB-flat.vmdk")
set(HDD_IMAGE "SWEB.qcow2")
set(INITRD_FILE ${PROJECT_BINARY_DIR}/sweb_initrd)
# set(HDD_IMAGE_FILE ${PROJECT_BINARY_DIR}/SWEB-flat.vmdk)

if ("${ARCH}" STREQUAL "")
  set (ARCH "x86/64")
endif("${ARCH}" STREQUAL "")
MESSAGE("-- Target architecture: ${ARCH}")

if ("${ARCH}" MATCHES "arm.*")
  add_definitions(-DARM)
endif("${ARCH}" MATCHES "arm.*")

unset(CMAKE_CROSSCOMPILING)
if(APPLE OR WIN32)
  set(CMAKE_CROSSCOMPILING 1)
  MESSAGE("-- Cross-Compiling on Apple/Windows")
endif(APPLE OR WIN32)

# add-dbg for userspace binaries
include(AddDebugInfo)

include(arch/${ARCH}/CMakeLists.compiler)



string(REPLACE "/" ";" ARCH_LIST ${ARCH})
string(REPLACE "/" "_" ARCH_ESC ${ARCH})
string(TOUPPER ${ARCH_ESC} ARCH_ESC)

add_definitions(-DCMAKE_${ARCH_ESC}=1)

list(LENGTH ARCH_LIST ARCH_DEPTH)

#Find program executables needed during compilation
find_program(LD_EXECUTABLE gcc)
find_program(OBJCOPY_EXECUTABLE objcopy)
find_program(DOXYGEN_EXECUTABLE doxygen)
find_program(STAT_EXECUTABLE stat)

set(ColourReset "")
set(BoldRed     "")
if(NOT WIN32)
  string(ASCII 27 Esc)
  set(ColourReset "${Esc}[m")
  set(BoldRed     "${Esc}[1;31m")
endif()
set(TMPFS_CHECK "none")
execute_process(COMMAND ${STAT_EXECUTABLE} --file-system --format=%T ${PROJECT_BINARY_DIR}
                OUTPUT_VARIABLE TMPFS_CHECK
                OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT("${TMPFS_CHECK}" STREQUAL "tmpfs"))
  MESSAGE("-- ${BoldRed}WARNING: build folder is not tmpfs - compilation will be slow and bad for the hard disk${ColourReset}")
endif(NOT("${TMPFS_CHECK}" STREQUAL "tmpfs"))

set(NOPIEFLAG -no-pie)
execute_process(COMMAND ${LD_EXECUTABLE} ${NOPIEFLAG}
                ERROR_VARIABLE PIE_CHECK
                ERROR_STRIP_TRAILING_WHITESPACE)
if("${PIE_CHECK}" MATCHES ".*unrecognized.*")
set(NOPIEFLAG )
endif()
set(NOPICFLAG -fno-PIC)
execute_process(COMMAND ${CMAKE_C_COMPILER} ${NOPICFLAG}
                ERROR_VARIABLE PIC_CHECK
                ERROR_STRIP_TRAILING_WHITESPACE)
if("${PIC_CHECK}" MATCHES ".*unrecognized.*")
set(NOPICFLAG )
endif()

#Initialize CMake output directories
set(LIBRARY_OUTPUT_PATH  "${PROJECT_BINARY_DIR}/lib")
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}")

include(arch/${ARCH}/CMakeLists.include)
include(arch/${ARCH}/CMakeLists.userspace)

if(CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSS_COMPILE_FLAGS -G "Unix Makefiles")
else(CMAKE_CROSSCOMPILING) # not cross compiling
  set(CMAKE_CROSS_COMPILE_FLAGS )
endif(CMAKE_CROSSCOMPILING)

if ("${DEBUG}" STREQUAL "1")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG=1")
endif()


# Searches for asm, c and cpp files and adds the library
function(ADD_PROJECT_LIBRARY LIBRARY_NAME)
  arch2obj(archobj_libname ${LIBRARY_NAME})

  file(GLOB source_files CONFIGURE_DEPENDS ${SOURCE_WILDCARDS})

  target_sources(kernel PUBLIC
    ${source_files})

  target_link_libraries(kernel PUBLIC
      ${archobj_libname})
endfunction(ADD_PROJECT_LIBRARY)

set (SOURCE_WILDCARDS *.cpp *.c *.S)

set(LIBRARY_FILENAMES)

# Initialize global (environment) variables
set(ENV{LIBRARY_NAMES})

# Build the Linker command
target_link_options(kernel_options INTERFACE
    -g -u entry -Wl,-T ${CMAKE_SOURCE_DIR}/arch/${ARCH}/utils/kernel-ld-script.ld ${ARCH_APPEND_LD_ARGUMENTS})


# Create target for userspace libc (Need to to that here because some files (e.g. syscall.c) are all over the place)
add_library(userspace_libc "")
target_compile_options(userspace_libc PUBLIC ${ARCH_USERSPACE_COMPILE_OPTIONS})

# Add the source directories
add_subdirectory(arch)
add_subdirectory(common)
add_subdirectory(utils)
add_subdirectory(userspace)

target_link_libraries(kernel_libc PUBLIC kernel_options)
target_link_libraries(kernel PUBLIC kernel_libc)

#FINAL_LIB_NAMES should contain the names of all libraries
#these names can be used to link the kernel, no unpacking of *.a files is needed anymore
set(FINAL_LIB_NAMES $ENV{LIBRARY_NAMES})

# Name of the executables of the userspace, needed for dependency checking
set(FINAL_USERSPACE_NAMES $ENV{USERSPACE_NAMES})





# Build userspace exe2minixfs command
set(MINIXFS_ARGUMENT "")
foreach(file $ENV{USERSPACE_NAMES_EXE2MINIX})
  list(APPEND MINIXFS_ARGUMENT ${file})
endforeach(file)

file(GLOB userspace_data CONFIGURE_DEPENDS userspace/data/*)
foreach(file ${userspace_data})
  get_filename_component(datafile ${file} NAME)
  list(APPEND MINIXFS_ARGUMENT ${file} ${datafile})
endforeach(file)



#Custom Target: hdd_image
#Creates the hd image and copies all files to it
add_custom_target (hdd_image ALL
    DEPENDS kernel_to_image userspace_to_image
    COMMAND qemu-img convert -f raw -O qcow2 ${HDD_IMAGE_RAW} ${HDD_IMAGE}
)

#Custom Command: invoke exe2minixfs and copy boot files to our hd image
add_custom_target (kernel_to_image
    DEPENDS blank_hdd_image kernel exe2minixfs mkminixfs userspace_to_image "${HDD_IMAGE_RAW}" "${CMAKE_SOURCE_DIR}/utils/images/menu.lst" # "${INITRD_FILE}"

    # x86/32 doesn't generate kernel.dbg
    COMMAND ${CMAKE_COMMAND} -E touch "./kernel.dbg"
    #COMMAND ./mkminixfs "${HDD_IMAGE_RAW}" 1
    COMMAND ./exe2minixfs "${HDD_IMAGE_RAW}" 1 "${CMAKE_SOURCE_DIR}/utils/images/menu.lst" boot/grub/menu.lst
                                           $<TARGET_FILE:kernel> boot/kernel.x
                                           ./kernel.dbg boot/kernel.dbg
                                           #"${INITRD_FILE}" boot/initrd
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMENT "Copying kernel files to image..."
)

add_custom_command (OUTPUT ${INITRD_FILE}
  DEPENDS mkminixfs exe2minixfs
  COMMAND dd if=/dev/zero of="${INITRD_FILE}" bs=512 count=1024
  COMMAND ./mkminixfs -d "${INITRD_FILE}"
  COMMAND ./exe2minixfs "${INITRD_FILE}" 0 ${CMAKE_SOURCE_DIR}/README.md README.md
  COMMENT "Creating empty initrd..."
  )

#Custom Command: invoke exe2minixfs and copy all userspace programs to our hd image second partition
add_custom_target (userspace_to_image
    DEPENDS blank_hdd_image ${FINAL_USERSPACE_NAMES} exe2minixfs mkminixfs "${HDD_IMAGE_RAW}" # "${INITRD_FILE}"
    COMMAND mkdir -p userspace/data
    COMMAND cp -f ${CMAKE_SOURCE_DIR}/userspace/data/* ${PROJECT_BINARY_DIR}/userspace/data
    # COMMAND ./mkminixfs -d "${HDD_IMAGE_RAW}" -p 2
    COMMAND ./exe2minixfs "${HDD_IMAGE_RAW}" 2 ${MINIXFS_ARGUMENT}
    # COMMAND ./exe2minixfs "${INITRD_FILE}" 0 ${MINIXFS_ARGUMENT}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMENT "Copying userspace programs to image..."
)


#Custom target: make bochs
#Run bochs in non debugging mode
add_custom_target(bochs
    COMMAND ${BOCHS_PATH} -q -f ${PROJECT_SOURCE_DIR}/utils/bochsrc
    COMMENT "Going to ${BOCHS_PATH} -f ${PROJECT_SOURCE_DIR}/utils/bochsrc"
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
)

#Custom target: make bochsgdb
#Run bochs in debugging mode
add_custom_target(bochsgdb
    COMMAND ${BOCHS_PATH} -q -f '${PROJECT_SOURCE_DIR}/utils/bochsrc' "gdbstub: enabled=1, port=1234"
    COMMENT "Going to ${BOCHS_PATH} -f ${PROJECT_SOURCE_DIR}/utils/bochsrc \"gdbstub: enabled=1, port=1234\""
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
)

#Custom target: make emu
#Run qemu in emu mode
add_custom_target(emu
    COMMAND cat '${PROJECT_SOURCE_DIR}/utils/emu.txt'
)

#Custom target: make runddd
#Run ddd with some presets
add_custom_target(runddd
    COMMAND ddd -cd ${PROJECT_SOURCE_DIR}/ -command ${PROJECT_SOURCE_DIR}/utils/gdbinit ${PROJECT_BINARY_DIR}/${KERNEL_BINARY}
    COMMENT "Going to ddd"
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
)

#Custom target: make runcgdb
#Run cgdb with some presets
add_custom_target(runcgdb
    COMMAND cgdb -cd ${PROJECT_SOURCE_DIR}/ -command ${PROJECT_SOURCE_DIR}/utils/gdbinit ${PROJECT_BINARY_DIR}/${KERNEL_BINARY}
    COMMENT "Going to cgdb"
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
)

#Custom target: make rungdbgui
#Run gdbgui with some presets
add_custom_target(rungdbgui
    COMMAND gdbgui -g 'gdb --ex "file ${PROJECT_BINARY_DIR}/${KERNEL_BINARY}" --command=${PROJECT_SOURCE_DIR}/utils/gdbinit' --project /tmp/sweb_upstream/ ${PROJECT_BINARY_DIR}/${KERNEL_BINARY}
    COMMENT "Going to gdbgui"
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
)


#Custom target: make rungdb
#Run gdb with some presets
add_custom_target(rungdb
    COMMAND gdb -cd ${PROJECT_SOURCE_DIR}/ -command ${PROJECT_SOURCE_DIR}/utils/gdbinit ${PROJECT_BINARY_DIR}/${KERNEL_BINARY}
    COMMENT "Going to gdb"
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
)

#Custom target: make mrproper
#Makes really clean (except for the object files)
add_custom_target(mrproper
    COMMAND make clean
    COMMAND ${PROJECT_SOURCE_DIR}/utils/prompt.sh "rm -fR ${PROJECT_BINARY_DIR}/*: remove all arguments recursively [Y/n]? "
    COMMAND rm -fR ${PROJECT_BINARY_DIR}/* || true
    COMMAND cmake -DARCH=${ARCH} ${PROJECT_SOURCE_DIR} ${CMAKE_CROSS_COMPILE_FLAGS}
)

#Custom target: make debug
add_custom_target(debug
    COMMAND make clean
    COMMAND ${PROJECT_SOURCE_DIR}/utils/prompt.sh "rm -fR ${PROJECT_BINARY_DIR}/*: remove all arguments recursively [Y/n]? "
    COMMAND rm -fR ${PROJECT_BINARY_DIR}/* || true
    COMMAND cmake -DDEBUG=1 -DARCH=${ARCH} ${PROJECT_SOURCE_DIR} ${CMAKE_CROSS_COMPILE_FLAGS}
)


# Define the "make ${ARCH}" targets
include(MakeArchTarget)

MAKE_ARCH_TARGET(x86_32 x86/32 "")
MAKE_ARCH_TARGET(x86_32_pae x86/32/pae "")
MAKE_ARCH_TARGET(x86_64 x86/64 "")

MAKE_ARCH_TARGET(arm_icp arm/integratorcp "")
MAKE_ARCH_TARGET(arm_rpi2 arm/rpi2 "")
MAKE_ARCH_TARGET(armv8_rpi3 armv8/rpi3 "-DVIRTUALIZED_QEMU=1")
MAKE_ARCH_TARGET(armv8_rpi3_hardware armv8/rpi3 "")



#Builds the doxygen documentation
add_custom_target(doxygen
    COMMAND mkdir -p \"${PROJECT_SOURCE_DIR}/doc\"
    COMMAND cp \"${PROJECT_SOURCE_DIR}/utils/sweb.dox\" \"${PROJECT_SOURCE_DIR}/doc/sweb_cmake.dox\"
    COMMAND sed -i -e 's/\\$$\\[DOC_OUTPUT_DIR\\]$$/docs/' '${PROJECT_SOURCE_DIR}/doc/sweb_cmake.dox'
    COMMAND ${DOXYGEN_EXECUTABLE} \"${PROJECT_SOURCE_DIR}/doc/sweb_cmake.dox\"
    COMMAND rm -f \"${PROJECT_SOURCE_DIR}/doc/sweb_cmake.dox\"
    COMMENT "Building SWEB documentation"
)

#Outputs help on build targets
add_custom_target(showhelp
    COMMAND printf \"\\nBuild target summary:\\n\\n\"
    COMMAND printf \"<default target>\\tbuild changed files, generate kernel,\\n\\t\\t\\textract hdd image, copy kernel\\n\\t\\t\\tand userspace file to image\\n\"
    COMMAND printf \"\\nclean\\t\\tclean up object files\\n\"
    COMMAND printf \"\\nmrproper\\tdeletes binary dir, cleans up,\\n\\t\\tregenerates all cmake specific files\\n\"
    COMMAND printf \"\\nbochs\\t\\truns bochs without gdb\\n\"
    COMMAND printf \"\\nbochsgdb\\truns bochs, waiting for gdb connection at localhost:1234\\n\"
    COMMAND printf \"\\nqemu\\t\\truns qemu, without gdb\\n\"
    COMMAND printf \"\\nqemugdb\\t\\truns qemu, waiting for gdb connection at localhost:1234\\n\"
    COMMAND printf \"\\nrunddd\\t\\truns ddd, connecting to localhost:1234\\n\"
    COMMAND printf \"\\nruncgdb\\t\\truns cgdb, connecting to localhost:1234\\n\"
    COMMAND printf \"\\nrungdb\\t\\truns gdb, connecting to localhost:1234\\n\"
    COMMAND printf \"\\ndoxygen\\t\\tbuilds the html and latex source documentation of SWEB\\n\"
    COMMAND printf \"\\nshowhelp\\t\\tshows this target summary\\n\"
    COMMAND printf \"\\n\\n\"
)
