if(COMMAND cmake_policy)
  cmake_policy(VERSION 2.6)
endif(COMMAND cmake_policy)
include(MakeOptions)

unset(CMAKE_CROSSCOMPILING)
if(APPLE OR WIN32)
  set(CMAKE_CROSSCOMPILING 1)
endif(APPLE OR WIN32)

include(arch/${ARCH}/CMakeLists.compiler)

project(sweb)

include(MakeOptions)

string(REPLACE "/" ";" ARCH_LIST ${ARCH})
string(REPLACE "/" "_" ARCH_ESC ${ARCH})
string(TOUPPER ${ARCH_ESC} ARCH_ESC)

add_definitions(-DCMAKE_${ARCH_ESC}=1)

list(LENGTH ARCH_LIST ARCH_DEPTH)

#Find program executables needed during compilation
find_program(ASM_EXECUTABLE nasm)
find_program(LD_EXECUTABLE cc)
find_program(AR_EXECUTABLE ar)
find_program(DOXYGEN_EXECUTABLE doxygen)
find_program(OBJCOPY_EXECUTABLE objcopy)

#Initialize CMake output directories
set(CMAKE_BINARY_DIR "${BIN_DIR_PREFIX}${BIN_DIR_NAME}")
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  set(PROJECT_BINARY_DIR "${PROJECT_BINARY_DIR}/${BIN_DIR_PREFIX}${BIN_DIR_NAME}")
endif(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
set(UTILS_DIR "${PROJECT_BINARY_DIR}/../sweb/utils")
set(LIBRARY_OUTPUT_PATH  "${PROJECT_BINARY_DIR}/lib")
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}")

include(arch/${ARCH}/CMakeLists.include)
cmake_minimum_required(VERSION 2.8)

if(CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSS_COMPILE_FLAGS -G "Unix Makefiles")
else(CMAKE_CROSSCOMPILING) # not cross compiling
  set(CMAKE_CROSS_COMPILE_FLAGS )
endif(CMAKE_CROSSCOMPILING)


# Macro which processes ASM files
# OBJS are the names (without extension) in the current directory
MACRO(ASM2OBJ OUTPUTOBJNAMES)

	foreach(src ${ARGN})
		set(ASM_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/${src}.s)
		set(ASM_OBJECT ${CMAKE_CURRENT_BINARY_DIR}/${src}.o)
		set(ASM_OBJECTS ${ASM_OBJECTS} ${ASM_OBJECT})

		add_custom_command(
		OUTPUT ${ASM_OBJECT}
		COMMAND ${ASM_EXECUTABLE}
		ARGS    ${ASM_FORMAT} -o ${ASM_OBJECT} ${ASM_SOURCE}
		DEPENDS ${ASM_SOURCE}
		)
	ENDFOREACH(src)

	set(${OUTPUTOBJNAMES} ${ASM_OBJECTS})
ENDMACRO(ASM2OBJ)

# Finds and returns ASM source files in the current directory
MACRO(FIND_ASM_FILES OUTPUTFILES)
	file(GLOB filenames ${ASM_WILDCARDS})

	set(TEMPOUTPUTLIST)
	foreach(src ${filenames})
		get_filename_component(TEMPOUTPUT ${src} NAME_WE)
		set(TEMPOUTPUTLIST ${TEMPOUTPUTLIST} ${TEMPOUTPUT})
	endforeach(src)
	set(${OUTPUTFILES} ${TEMPOUTPUTLIST})
ENDMACRO(FIND_ASM_FILES)

# Searches for ASM files, Searches for c and cpp files
# compiles ASM files and adds the library
MACRO(ADD_PROJECT_LIBRARY LIBRARY_NAME)
	find_asm_files(asm_source)

	asm2obj(asm_objs ${asm_source})

	file(GLOB source_files ${SOURCE_WILDCARDS})

	set(library_files)

	if(source_files)
		set(library_files ${source_files})
	endif(source_files)

	if(asm_objs)
		set(library_files ${library_files} ${asm_objs})
	endif(asm_objs)

	if(library_files)
		add_library(${LIBRARY_NAME} ${library_files})
		set(ENV{LIBRARY_NAMES} "$ENV{LIBRARY_NAMES};${LIBRARY_NAME}")
	endif(library_files)
ENDMACRO(ADD_PROJECT_LIBRARY)

set (SOURCE_WILDCARDS *.cpp *.c *.tcpp *.S)
set (ASM_WILDCARDS *.s)
set (INFO_FILE info.file)


if(NOT BIN_DIR_PREFIX)
	message(FATAL_ERROR "BIN_DIR_PREFIX not set, check MakeOptions file")
endif(NOT BIN_DIR_PREFIX)

if(NOT BIN_DIR_NAME)
	message(FATAL_ERROR "BIN_DIR_NAME not set, check MakeOptions file")
endif(NOT BIN_DIR_NAME)

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
set(LD_ARGUMENT ${LD_ARGUMENT} -Wl,-Map -Wl,${PROJECT_BINARY_DIR}/kernel.map)

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
	DEPENDS ${FINAL_LIB_NAMES} sweb-img-util
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
	COMMAND cp "${CMAKE_SOURCE_DIR}/utils/images/menu.lst.hda" "./temp/boot/grub/menu.lst"
	COMMAND	cp "${CMAKE_SOURCE_DIR}/utils/images/SWEB.vmdk" "./"
	COMMAND cp "${CMAKE_SOURCE_DIR}/utils/images/sweb.vmx" "./"
	COMMAND cp "${CMAKE_SOURCE_DIR}/utils/images/nvram" "./"

	WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
	COMMENT "Copying helper files..."
	)

#Custom Command: invoke exe2minixfs and copy the temp dir to our hd image
add_custom_command (OUTPUT copy_to_image
	DEPENDS helper_files sweb-img-util
	COMMAND ./sweb-img-util -x SWEB-flat.vmdk 0 ./kernel.x boot/kernel.x ./temp/boot/grub/menu.lst boot/grub/menu.lst
	COMMAND rm -fR temp

	WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
	COMMENT "Copying files to image..."
	)

#Custom Command: invoke exe2minixfs and copy all userspace programs to our hd image second partition
add_custom_command (OUTPUT userspace_to_image
	DEPENDS hdd_image ${FINAL_USERSPACE_NAMES}
	COMMAND mkdir -p userspace/data
  COMMAND cp -f ${CMAKE_SOURCE_DIR}/userspace/data/* ${PROJECT_BINARY_DIR}/userspace/data
 	COMMAND ./sweb-img-util -x SWEB-flat.vmdk 1 ${MINIXFS_ARGUMENT}
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

#Custom target: make format
#invoke sweb-img-util and re-formats the user-progs partition
add_custom_target(format
	COMMAND ./sweb-img-util -f SWEB-flat.vmdk 1 minix 0 1 30
	WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
	COMMENT "(re)format image's user-progs partition..."
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
	COMMAND ${PROJECT_SOURCE_DIR}/utils/prompt.sh "rm -fR ${PROJECT_BINARY_DIR}/*: remove all arguments recursively [Y/n]? "
	COMMAND rm -fR ${PROJECT_BINARY_DIR}/* || true
  COMMAND cmake ${PROJECT_SOURCE_DIR} ${CMAKE_CROSS_COMPILE_FLAGS}
	)

#Custom target: make x86_32
#Changes MakeOptions ARCH setting to x86_32
add_custom_target(x86_32
        COMMENT "sed -i 's/SET (ARCH .*/SET (ARCH x86/32)/' ${PROJECT_SOURCE_DIR}/MakeOptions"
        COMMAND sed -i "s/SET \\(ARCH .*/SET \\(ARCH x86\\\\/32\\)/" ${PROJECT_SOURCE_DIR}/MakeOptions
        COMMAND make mrproper
        )

#Custom target: make x86_32_pae
#Changes MakeOptions ARCH setting to x86_32_pae
add_custom_target(x86_32_pae
        COMMENT "sed -i 's/SET (ARCH .*/SET (ARCH x86/32/pae)/' ${PROJECT_SOURCE_DIR}/MakeOptions"
        COMMAND sed -i "s/SET \\(ARCH .*/SET \\(ARCH x86\\\\/32\\\\/pae\\)/" ${PROJECT_SOURCE_DIR}/MakeOptions
        COMMAND make mrproper
        )

#Custom target: make x86_64
#Changes MakeOptions ARCH setting to x86_64
add_custom_target(x86_64
        COMMENT "sed -i 's/SET (ARCH .*/SET (ARCH x86/64)/' ${PROJECT_SOURCE_DIR}/MakeOptions"
        COMMAND sed -i "s/SET \\(ARCH .*/SET \\(ARCH x86\\\\/64\\)/" ${PROJECT_SOURCE_DIR}/MakeOptions
        COMMAND make mrproper
        )

#Custom target: make arm_icp
#Changes MakeOptions ARCH setting to arm_icp
add_custom_target(arm_icp
        COMMENT "sed -i 's/SET (ARCH .*/SET (ARCH arm/integratorcp)/' ${PROJECT_SOURCE_DIR}/MakeOptions"
        COMMAND sed -i "s/SET \\(ARCH .*/SET \\(ARCH arm\\\\/integratorcp\\)/" ${PROJECT_SOURCE_DIR}/MakeOptions
        COMMAND make mrproper
        )

#Custom target: make arm_verdex
#Changes MakeOptions ARCH setting to arm_verdex
add_custom_target(arm_verdex
        COMMENT "sed -i 's/SET (ARCH .*/SET (ARCH arm/verdex)/' ${PROJECT_SOURCE_DIR}/MakeOptions"
        COMMAND sed -i "s/SET \\(ARCH .*/SET \\(ARCH arm\\\\/verdex\\)/" ${PROJECT_SOURCE_DIR}/MakeOptions
        COMMAND make mrproper
        )

#Custom target: make arm_rpi
#Changes MakeOptions ARCH setting to arm_rpi
add_custom_target(arm_rpi
        COMMENT "sed -i 's/SET (ARCH .*/SET (ARCH arm/rpi)/' ${PROJECT_SOURCE_DIR}/MakeOptions"
        COMMAND sed -i "s/SET \\(ARCH .*/SET \\(ARCH arm\\\\/rpi\\)/" ${PROJECT_SOURCE_DIR}/MakeOptions
        COMMAND make mrproper
        )

#Builds the doxygen documentation
add_custom_target(doxygen
	COMMAND mkdir -p \"${PROJECT_SOURCE_DIR}/doc\"
	COMMAND cp \"${PROJECT_SOURCE_DIR}/utils/sweb.dox\" \"${PROJECT_SOURCE_DIR}/doc/sweb_cmake.dox\"
	COMMAND sed -i -e 's/\\$$\\[DOC_OUTPUT_DIR\\]$$/${DOC_DIR}/' '${PROJECT_SOURCE_DIR}/doc/sweb_cmake.dox'
	COMMAND ${DOXYGEN_EXECUTABLE} \"${PROJECT_SOURCE_DIR}/doc/sweb_cmake.dox\"
	COMMAND rm -f \"${PROJECT_SOURCE_DIR}/doc/sweb_cmake.dox\"
	COMMENT "Building SWEB documentation"
	)

#Outputs help on build targets
add_custom_target(help
	COMMAND printf \"\\nBuild target summary:\\n\\n\"
	COMMAND printf \"<default target>\\tbuild changed files, generate kernel,\\n\\t\\t\\textract hdd image, copy kernel\\n\\t\\t\\tand userspace file to image\\n\"
	COMMAND printf \"\\nuserspace\\tbuild userspace programs and copy to hdd image\\n\"
	COMMAND printf \"\\nclean\\t\\tclean up object files\\n\"
	COMMAND printf \"\\nmrproper\\tdeletes binary dir, cleans up,\\n\\t\\tregenerates all cmake specific files\\n\"
	COMMAND printf \"\\nremove_cmake\\tremoves all files generated by cmake\\n\"
	COMMAND printf \"\\nbochs\\t\\truns bochs without gdb\\n\"
	COMMAND printf \"\\nbochsgdb\\truns bochs, waiting for gdb connection at localhost:1234\\n\"
	COMMAND printf \"\\nqemu\\t\\truns qemu, without gdb\\n\"
	COMMAND printf \"\\nqemugdb\\t\\truns qemu, waiting for gdb connection at localhost:1234\\n\"
	COMMAND printf \"\\nrunddd\\t\\truns ddd, connecting to localhost:1234\\n\"
	COMMAND printf \"\\nruncgdb\\t\\truns cgdb, connecting to localhost:1234\\n\"
	COMMAND printf \"\\nrungdb\\t\\truns gdb, connecting to localhost:1234\\n\"
	COMMAND printf \"\\nsubmit-nogit assignment=<1 or 2> group=<group number, upper case>\\n\\t\\tBuilds a submit package without \\n\\t\\tworrying about git \(EMERGENCY ONLY!\)\\n\"
	COMMAND printf \"\\ninfo\\t\\toutputs some important system informations\\n\"
	COMMAND printf \"\\ndoxygen\\t\\tbuilds the html and latex source documentation of SWEB\\n\"
	COMMAND printf \"\\nhelp\\t\\tshows this target summary\\n\"
	COMMAND printf \"\\n\\n\"
	)
