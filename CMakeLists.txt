cmake_minimum_required(VERSION 2.8)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
if(COMMAND cmake_policy)
  cmake_policy(VERSION 2.6)
endif(COMMAND cmake_policy)
if(POLICY CMP0037)
  cmake_policy(SET CMP0037 OLD)
endif(POLICY CMP0037)

if ("${ARCH}" STREQUAL "")
  set (ARCH "x86/64")
endif("${ARCH}" STREQUAL "")
MESSAGE("-- Target architecture: ${ARCH}")

if ("${BIN_DIR_PREFIX}" STREQUAL "")
  SET (BIN_DIR_PREFIX "../")
endif ("${BIN_DIR_PREFIX}" STREQUAL "")

if ("${BIN_DIR_NAME}" STREQUAL "")
  SET (BIN_DIR_NAME "sweb-bin")
endif ("${BIN_DIR_NAME}" STREQUAL "")

if ("${DOC_DIR}" STREQUAL "")
  SET (DOC_DIR "..\\/sweb-docs")
endif ("${DOC_DIR}" STREQUAL "")
unset(CMAKE_CROSSCOMPILING)
if(APPLE OR WIN32)
  set(CMAKE_CROSSCOMPILING 1)
  MESSAGE("-- Cross-Compiling on Apple/Windows")
endif(APPLE OR WIN32)

include(arch/${ARCH}/CMakeLists.compiler)

enable_language(ASM)

project(sweb)

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
                ERROR_VARIABLE PIE_CHECK
                ERROR_STRIP_TRAILING_WHITESPACE)
if("${PIC_CHECK}" MATCHES ".*unrecognized.*")
set(NOPICFLAG )
endif()

#Initialize CMake output directories
set(UTILS_DIR "${PROJECT_BINARY_DIR}/../sweb/utils")
set(LIBRARY_OUTPUT_PATH  "${PROJECT_BINARY_DIR}/lib")
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}")

include(arch/${ARCH}/CMakeLists.include)
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS}")

if(CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSS_COMPILE_FLAGS -G "Unix Makefiles")
else(CMAKE_CROSSCOMPILING) # not cross compiling
  set(CMAKE_CROSS_COMPILE_FLAGS )
endif(CMAKE_CROSSCOMPILING)

if ("${DEBUG}" STREQUAL "1")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG=1")
endif()


# Searches for asm, c and cpp files and adds the library
MACRO(ADD_PROJECT_LIBRARY LIBRARY_NAME)
	arch2obj(arch_objs)

	file(GLOB source_files ${SOURCE_WILDCARDS})

	set(library_files)

	if(source_files)
		set(library_files ${source_files})
	endif(source_files)

  if(arch_objs)
    set(library_files ${library_files} ${arch_objs})
  endif(arch_objs)

	if(library_files)
		add_library(${LIBRARY_NAME} ${library_files})
		set(ENV{LIBRARY_NAMES} "$ENV{LIBRARY_NAMES};${LIBRARY_NAME}")
	endif(library_files)
ENDMACRO(ADD_PROJECT_LIBRARY)

set (SOURCE_WILDCARDS *.cpp *.c *.S)
set (INFO_FILE info.file)

set(LIBRARY_FILENAMES)

#Initialize global (environment) variables
set(ENV{LIBRARY_NAMES})

#Add the source directories
add_subdirectory(arch)
add_subdirectory(common)
add_subdirectory(utils)
add_subdirectory(userspace)


#FINAL_LIB_NAMES should contain the names of all libraries
#these names can be used to link the kernel, no unpacking of *.a files is needed anymore
set(FINAL_LIB_NAMES $ENV{LIBRARY_NAMES})

#Name of the executables of the userspace, needed for dependency checking
set(FINAL_USERSPACE_NAMES $ENV{USERSPACE_NAMES})



#Build the Linker command
set(LD_ARGUMENT ${LD_ARGUMENT} -g -u entry -Wl,-T ${CMAKE_SOURCE_DIR}/arch/${ARCH}/utils/kernel-ld-script.ld)
set(LD_ARGUMENT ${LD_ARGUMENT} -o ${PROJECT_BINARY_DIR}/kernel.x)
#set(LD_ARGUMENT ${LD_ARGUMENT} -Wl,-Map -Wl,${PROJECT_BINARY_DIR}/kernel.map)

set(LD_ARGUMENT ${LD_ARGUMENT} -Wl,--start-group)
foreach(libfile ${FINAL_LIB_NAMES})
set(LD_ARGUMENT ${LD_ARGUMENT} ${LIBRARY_OUTPUT_PATH}/lib${libfile}.a)
endforeach(libfile)
set(LD_ARGUMENT ${LD_ARGUMENT} -Wl,--end-group)
set(LD_ARGUMENT ${LD_ARGUMENT} ${APPEND_LD_ARGUMENTS})

#Build userspace exe2minixfs command
set(MINIXFS_ARGUMENT "")
foreach(file $ENV{USERSPACE_NAMES_EXE2MINIX})
	set(MINIXFS_ARGUMENT ${MINIXFS_ARGUMENT} ${file})
endforeach(file)

file(GLOB userspace_data userspace/data/*)
foreach(file ${userspace_data})
    get_filename_component(datafile ${file} NAME)
    set(MINIXFS_ARGUMENT  ${MINIXFS_ARGUMENT} ${file} ${datafile})
endforeach(file)

#Custom Command: Outputs kernel_image
#Executes the linker command after all libraries where build successfully
add_custom_command (OUTPUT kernel_image
	DEPENDS ${FINAL_LIB_NAMES} exe2minixfs add-debug
	COMMAND ${LD_EXECUTABLE} ${LD_ARGUMENT}
  ${KERNEL_IMAGE_OBJCOPY}
	WORKING_DIRECTORY ${LIBRARY_OUTPUT_PATH}
	)

#Custom Target: kernel
#Generates kernel target, which is always executed
add_custom_target (kernel ALL
	DEPENDS kernel_image
	)

#Custom Command: Outputs bochs image
#Generates a image usable by bochs
add_custom_command (OUTPUT temp_dir
	#Create folder structure for the disk image
	COMMAND rm -rf "./temp"
	COMMAND mkdir "./temp"
	COMMAND mkdir "./temp/boot"
	COMMAND mkdir "./temp/boot/grub"
  COMMAND cp "./kernel.x" "./temp/boot"
  COMMAND touch "./kernel.dbg"
  COMMAND cp "./kernel.dbg" "./temp/boot"

	WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
	COMMENT "Creating temp dir..."
	)

#Custom Command: Outputs empty ext2 disk image
add_custom_command (OUTPUT hdd_image
	DEPENDS temp_dir
        COMMAND ${PROJECT_SOURCE_DIR}/utils/images/extract_image.sh
        ARGS ${PROJECT_SOURCE_DIR}
	WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
	COMMENT "Unpacking disk image..."
	)


#Custom Command: Writes helper files to temp file structure
add_custom_command (OUTPUT helper_files
	DEPENDS hdd_image
	COMMAND cp "${CMAKE_SOURCE_DIR}/utils/images/menu.lst" "./temp/boot/grub/menu.lst"

	WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
	COMMENT "Copying helper files..."
	)

#Custom Command: invoke exe2minixfs and copy the temp dir to our hd image
add_custom_command (OUTPUT copy_to_image
  DEPENDS helper_files exe2minixfs
  COMMAND ./exe2minixfs SWEB-flat.vmdk 32256 ./temp/boot/grub/menu.lst boot/grub/menu.lst ./kernel.x boot/kernel.x ./kernel.dbg boot/kernel.dbg 
  COMMAND rm -R temp

  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  COMMENT "Copying files to image..."
  )

#Custom Command: compile add-debug to add debug infos to binaries
add_custom_command (OUTPUT add-debug
    COMMENT "Building add-debug"
    COMMAND make -C "${CMAKE_SOURCE_DIR}/utils/add-debug"
    COMMAND cp "${CMAKE_SOURCE_DIR}/utils/add-debug/add-dbg" ${PROJECT_BINARY_DIR}
    COMMAND cp "${CMAKE_SOURCE_DIR}/utils/add-debug/add-dbg.sh" ${PROJECT_BINARY_DIR}
  )
  
#Custom Command: invoke exe2minixfs and copy all userspace programs to our hd image second partition
add_custom_command (OUTPUT userspace_to_image
	DEPENDS hdd_image add-debug ${FINAL_USERSPACE_NAMES}
	COMMAND mkdir -p userspace/data
  COMMAND cp -f ${CMAKE_SOURCE_DIR}/userspace/data/* ${PROJECT_BINARY_DIR}/userspace/data
  COMMAND ./add-dbg.sh ${OBJCOPY_EXECUTABLE} ${MINIXFS_ARGUMENT}
  COMMAND ./exe2minixfs SWEB-flat.vmdk 10321920 ${MINIXFS_ARGUMENT}
	WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
	COMMENT "Copying userspace programs to image..."
	)

#Custom Target: install
#Creates the hd image and copies all files to it
add_custom_target (install ALL
	DEPENDS copy_to_image userspace_to_image
	)

#The kernel must be built before installing
add_dependencies(install kernel)

add_custom_target (userspace
	DEPENDS userspace_to_image
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
	COMMAND	cat '${PROJECT_SOURCE_DIR}/utils/emu.txt'
	WORKING_DIRECTORY ${UTILS_DIR}
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

#Custom target: make x86_32
add_custom_target(x86_32
        COMMAND ${PROJECT_SOURCE_DIR}/utils/prompt.sh "rm -fR ${PROJECT_BINARY_DIR}/*: remove all arguments recursively [Y/n]? "
        COMMAND rm -fR ${PROJECT_BINARY_DIR}/* || true
        COMMAND cmake -DARCH=x86/32 ${PROJECT_SOURCE_DIR} ${CMAKE_CROSS_COMPILE_FLAGS}
        )

#Custom target: make x86_32_pae
add_custom_target(x86_32_pae
        COMMAND ${PROJECT_SOURCE_DIR}/utils/prompt.sh "rm -fR ${PROJECT_BINARY_DIR}/*: remove all arguments recursively [Y/n]? "
        COMMAND rm -fR ${PROJECT_BINARY_DIR}/* || true
        COMMAND cmake -DARCH=x86/32/pae ${PROJECT_SOURCE_DIR} ${CMAKE_CROSS_COMPILE_FLAGS}
        )

#Custom target: make x86_64
add_custom_target(x86_64
        COMMAND ${PROJECT_SOURCE_DIR}/utils/prompt.sh "rm -fR ${PROJECT_BINARY_DIR}/*: remove all arguments recursively [Y/n]? "
        COMMAND rm -fR ${PROJECT_BINARY_DIR}/* || true
        COMMAND cmake -DARCH=x86/64 ${PROJECT_SOURCE_DIR} ${CMAKE_CROSS_COMPILE_FLAGS}
        )

#Custom target: make arm_icp
add_custom_target(arm_icp
        COMMAND ${PROJECT_SOURCE_DIR}/utils/prompt.sh "rm -fR ${PROJECT_BINARY_DIR}/*: remove all arguments recursively [Y/n]? "
        COMMAND rm -fR ${PROJECT_BINARY_DIR}/* || true
        COMMAND cmake -DARCH=arm/integratorcp ${PROJECT_SOURCE_DIR} ${CMAKE_CROSS_COMPILE_FLAGS}
        )

#Custom target: make arm_verdex
add_custom_target(arm_verdex
        COMMAND ${PROJECT_SOURCE_DIR}/utils/prompt.sh "rm -fR ${PROJECT_BINARY_DIR}/*: remove all arguments recursively [Y/n]? "
        COMMAND rm -fR ${PROJECT_BINARY_DIR}/* || true
        COMMAND cmake -DARCH=arm/verdex ${PROJECT_SOURCE_DIR} ${CMAKE_CROSS_COMPILE_FLAGS}
        )

#Custom target: make arm_rpi
add_custom_target(arm_rpi
        COMMAND ${PROJECT_SOURCE_DIR}/utils/prompt.sh "rm -fR ${PROJECT_BINARY_DIR}/*: remove all arguments recursively [Y/n]? "
        COMMAND rm -fR ${PROJECT_BINARY_DIR}/* || true
        COMMAND cmake -DARCH=arm/rpi ${PROJECT_SOURCE_DIR} ${CMAKE_CROSS_COMPILE_FLAGS}
        )

#Custom target: make arm_rpi2
add_custom_target(arm_rpi2
		COMMAND ${PROJECT_SOURCE_DIR}/utils/prompt.sh "rm -fR ${PROJECT_BINARY_DIR}/*: remove all arguments recursively [Y/n]? "
		COMMAND rm -fR ${PROJECT_BINARY_DIR}/* || true
		COMMAND cmake -DARCH=arm/rpi -DRPI2=1 ${PROJECT_SOURCE_DIR} ${CMAKE_CROSS_COMPILE_FLAGS}
		)

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
add_custom_target(help
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
	COMMAND printf \"\\nhelp\\t\\tshows this target summary\\n\"
	COMMAND printf \"\\n\\n\"
	)
