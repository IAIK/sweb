project(sweb)

include(MakeOptions)

cmake_minimum_required(VERSION 2.4)


# Macro which processes NASM files
# OBJS are the names (without extension) in the current directory
MACRO(ASM2OBJ OUTPUTOBJNAMES)

	foreach(src ${ARGN})
		set(ASM_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/${src}.s)
		set(ASM_OBJECT ${CMAKE_CURRENT_BINARY_DIR}/${src}.o)
		set(ASM_OBJECTS ${ASM_OBJECTS} ${ASM_OBJECT})

		add_custom_command(
		OUTPUT ${ASM_OBJECT}
		COMMAND ${NASM_EXECUTABLE}
		ARGS    -f elf -o ${ASM_OBJECT} ${ASM_SOURCE}
		DEPENDS ${ASM_SOURCE}
		)
	ENDFOREACH(src)

	set(${OUTPUTOBJNAMES} ${ASM_OBJECTS})
ENDMACRO(ASM2OBJ)

# Finds and returns NASM source files in the current directory
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


#Find program executables needed during compilation
find_program(NASM_EXECUTABLE nasm)
find_program(LD_EXECUTABLE ld)
find_program(AR_EXECUTABLE ar)
find_program(DOXYGEN_EXECUTABLE doxygen)


if(NOT BIN_DIR_PREFIX)
	message(FATAL_ERROR "BIN_DIR_PREFIX not set, check MakeOptions file")
endif(NOT BIN_DIR_PREFIX)

if(NOT BIN_DIR_NAME)
	message(FATAL_ERROR "BIN_DIR_NAME not set, check MakeOptions file")
endif(NOT BIN_DIR_NAME)


#Set Compiler and compiterflags
#set(CMAKE_CXX_COMPILER g++)
#set(CMAKE_C_COMPILER gcc)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -gstabs2 -O0  -Wno-deprecated -Wall -W -nostdinc -fno-builtin -nostdlib -fno-rtti -nostdinc++ -fno-exceptions -fno-stack-protector -Wall -Wextra -nostdlib -nostartfiles -nodefaultlibs")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0  -gstabs2 -Wall -W -nostdinc -fno-builtin -nostdlib -fno-exceptions -fno-stack-protector -Wall -Wextra  -nostdlib -nostartfiles -nodefaultlibs")

if(GGDB)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -O0 -ggdb -Wall -Wextra -Wno-deprecated -nostdinc -nostdlib -nostartfiles -nodefaultlibs -nostdinc++ -fno-builtin -fno-rtti -fno-exceptions -fno-stack-protector")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}     -m32 -O0 -ggdb -Wall -Wextra                 -nostdinc -nostdlib -nostartfiles -nodefaultlibs             -fno-builtin           -fno-exceptions -fno-stack-protector")
endif(GGDB)
if(NOT GGDB)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -O0 -gstabs+ -Wall -Wextra -Wno-deprecated -nostdinc -nostdlib -nostartfiles -nodefaultlibs -nostdinc++ -fno-builtin -fno-rtti -fno-exceptions -fno-stack-protector")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}     -m32 -O0 -gstabs+ -Wall -Wextra                 -nostdinc -nostdlib -nostartfiles -nodefaultlibs             -fno-builtin           -fno-exceptions -fno-stack-protector")
endif(NOT GGDB)


set(LIBRARY_FILENAMES)

#Initialize global (environment) variables
set(ENV{LIBRARY_NAMES})

#Initialize CMake output directories
set(CMAKE_BINARY_DIR "${BIN_DIR_PREFIX}${BIN_DIR_NAME}")
set(PROJECT_BINARY_DIR "${PROJECT_BINARY_DIR}/${BIN_DIR_PREFIX}${BIN_DIR_NAME}")
set(UTILS_DIR "${PROJECT_BINARY_DIR}/../sweb/utils")
set(LIBRARY_OUTPUT_PATH  "${PROJECT_BINARY_DIR}/lib")
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}")

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
set(LD_ARGUMENT -melf_i386)
set(LD_ARGUMENT ${LD_ARGUMENT} -g -u entry -T ${CMAKE_SOURCE_DIR}/arch/x86/utils/kernel-ld-script.ld)
set(LD_ARGUMENT ${LD_ARGUMENT} -o ${PROJECT_BINARY_DIR}/kernel.x)
set(LD_ARGUMENT ${LD_ARGUMENT} -Map ${PROJECT_BINARY_DIR}/kernel.map)

set(LD_ARGUMENT ${LD_ARGUMENT} --start-group)
foreach(libfile ${FINAL_LIB_NAMES})
	set(LD_ARGUMENT ${LD_ARGUMENT} ${LIBRARY_OUTPUT_PATH}/lib${libfile}.a)
endforeach(libfile)
set(LD_ARGUMENT ${LD_ARGUMENT} --end-group)


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
	DEPENDS ${FINAL_LIB_NAMES} exe2minixfs
	#DEPENDS ${FINAL_LIB_NAMES} exe2minixfs exe2pseudofs
	COMMAND ${LD_EXECUTABLE} ${LD_ARGUMENT}
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

	#COMMAND cp "../images/menu.lst.hda" "./temp/boot/grub"
	#COMMAND test -e "./ramfs" || cp "../images/ramfs" "./ramfs"
	COMMAND cp "./kernel.x" "./temp/boot"
	#COMMAND cp "./ramfs" "./temp/boot"

	WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
	COMMENT "Creating temp dir..."
	)

#Custom Command: Outputs empty ext2 disk image
add_custom_command (OUTPUT hdd_image
	DEPENDS temp_dir
	COMMAND if [ ! -f ./SWEB-flat.vmdk ]; then cp "${CMAKE_SOURCE_DIR}/images/SWEB-flat.vmdk.gz" "." \; gzip -df "./SWEB-flat.vmdk.gz" \; fi
	WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
	COMMENT "Unpacking disk image..."
	)


#Custom Command: Writes helper files to temp file structure
add_custom_command (OUTPUT helper_files
	DEPENDS hdd_image
	COMMAND cp "${CMAKE_SOURCE_DIR}/images/menu.lst.hda" "./temp/boot/grub/menu.lst"
	COMMAND	cp "${CMAKE_SOURCE_DIR}/images/SWEB.vmdk" "./"
	COMMAND cp "${CMAKE_SOURCE_DIR}/images/sweb.vmx" "./"
	#COMMAND	cp "../images/SWEB-minix.vmdk" "./"
	#COMMAND cp "../images/SWEB-flat-minix.vmdk.gz" "./"
	#COMMAND gzip -df "./SWEB-flat-minix.vmdk.gz"
	COMMAND cp "${CMAKE_SOURCE_DIR}/images/nvram" "./"

	WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
	COMMENT "Copying helper files..."
	)

#Custom Command: invoke exe2minixfs and copy the temp dir to our hd image
add_custom_command (OUTPUT copy_to_image
	DEPENDS helper_files exe2minixfs
	COMMAND ./exe2minixfs SWEB-flat.vmdk 32256 ./kernel.x /boot/kernel.x ./temp/boot/grub/menu.lst /boot/grub/menu.lst
	#./temp/boot/ramfs /boot/ramfs
	COMMAND rm -R temp

	WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
	COMMENT "Copying files to image..."
	)

#Custom Command: invoke exe2minixfs and copy all userspace programs to our hd image second partition
add_custom_command (OUTPUT userspace_to_image
	DEPENDS hdd_image ${FINAL_USERSPACE_NAMES}
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

#Checks for bochs configuration and runs 'make bochsconfig' if configuration does not exist
add_custom_command(OUTPUT check_for_bochs_config
	COMMAND if [ -f '${PROJECT_SOURCE_DIR}/utils/bochs/bochsrc' ]\; then
		printf \"\\nBochs configuration already exists, skipping 'make bochsconfig'\\n\\n\"\;
	else
		printf \"\\nBochs configuration does not exist, running 'make bochsconfig'\\n\\n\"\;
		make bochsconfig\;
	fi\;


	COMMENT "Checking for bochs configuration"
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	)

#Custom target: make bochs
#Run bochs in non debugging mode
add_custom_target(bochs
	DEPENDS check_for_bochs_config
	COMMAND ${BOCHS_PATH} -q -f ${PROJECT_SOURCE_DIR}/utils/bochs/bochsrc
	COMMENT "Going to ${BOCHS_PATH} -f ${PROJECT_SOURCE_DIR}/utils/bochs/bochsrc"
	WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
	)

#Custom target: make bochsgdb
#Run bochs in debugging mode
add_custom_target(bochsgdb
	DEPENDS check_for_bochs_config
	COMMAND ${BOCHS_PATH} -q -f '${PROJECT_SOURCE_DIR}/utils/bochs/bochsrc' "gdbstub: enabled=1, port=1234"
	COMMENT "Going to ${BOCHS_PATH} -f ${PROJECT_SOURCE_DIR}/utils/bochs/bochsrc \"gdbstub: enabled=1, port=1234\""
	WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
	)

#Custom target: make kvm
#Run kvm in non debugging mode
add_custom_target(kvm
        COMMAND qemu-system-i386 -m 8M -cpu kvm32 -hda SWEB-flat.vmdk -debugcon /dev/stdout
        COMMENT "Going to qemu-system-i386 -m 8M -cpu kvm32 -hda SWEB-flat.vmdk -debugcon /dev/stdout"
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        COMMAND reset -I
        )

#Custom target: make qemu
#Run qemu in non debugging mode
add_custom_target(qemu
	COMMAND	qemu-system-i386 -m 8M -hda SWEB-flat.vmdk -debugcon /dev/stdout
	COMMENT "Going to qemu-system-i386 -m 8M -hda SWEB-flat.vmdk -debugcon /dev/stdout"
	WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
	COMMAND reset -I
	)

#Custom target: make emu
#Run qemu in emu mode
add_custom_target(emu
	COMMAND	cat '${PROJECT_SOURCE_DIR}/utils/emu.txt'
	WORKING_DIRECTORY ${UTILS_DIR}
	)

#Custom target: make qemu
#Run qemu in non debugging mode
add_custom_target(qemugdb
	COMMAND	qemu -s -S -m 8M -hda SWEB-flat.vmdk -debugcon /dev/stdout
	COMMENT "Going to gdb qemu -s -S -m 8M -hda SWEB-flat.vmdk -debugcon /dev/stdout on localhost:1234"
	WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
	COMMAND reset -I
	)

#Custom target: make qemutacos
#Run qemu in pipe monitor mode for tacos
add_custom_target(qemutacos
  COMMAND qemu -hda SWEB-flat.vmdk -m 8M -snapshot -monitor pipe:qemu -nographic -debugcon /dev/stdout
  COMMENT "Going to qemu -hda SWEB-flat.vmdk -m 8M -snapshot -monitor pipe:qemu -nographic -debugcon /dev/stdout"
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  #COMMAND reset -I
  )

#Custom target: make runddd
#Run ddd with some presets
add_custom_target(runddd
	COMMAND ddd -cd ${PROJECT_SOURCE_DIR}/ -command ${PROJECT_SOURCE_DIR}/utils/gdb/gdbinit ${PROJECT_BINARY_DIR}/kernel.x
	COMMENT "Going to ddd"
	WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
	)

#Custom target: make runcgdb
#Run cgdb with some presets
add_custom_target(runcgdb
        COMMAND cgdb -cd ${PROJECT_SOURCE_DIR}/ -command ${PROJECT_SOURCE_DIR}/utils/gdb/gdbinit ${PROJECT_BINARY_DIR}/kernel.x
        COMMENT "Going to cgdb"
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        )


#Custom target: make rungdb
#Run gdb with some presets
add_custom_target(rungdb
	COMMAND gdb -cd ${PROJECT_SOURCE_DIR}/ -command ${PROJECT_SOURCE_DIR}/utils/gdb/gdbinit ${PROJECT_BINARY_DIR}/kernel.x
	COMMENT "Going to gdb"
	WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
	)

#Custom target: make debug
#debug target for gdb
add_custom_target(debug
	COMMAND cmake -D GGDB=1 .
        COMMAND touch CMakeFiles/is_debug_build
	COMMAND make
  )

#Custom target: make mrproper
#Makes really clean
add_custom_target(mrproper
	COMMAND rm -r \"${PROJECT_BINARY_DIR}/\"
	COMMAND cmake .
	)

#Enables relocation of the source files (moving from /home/user1/sweb -> /home/user1/foo/.. )
add_custom_target(remove_cmake
	COMMAND make clean;
	COMMAND find . -name 'CMakeCache.txt' -exec rm {} +;
	COMMAND find . -name 'CMakeFiles' -exec rm -f -r {} +;
	COMMAND find . -name 'cmake_install.cmake' -exec rm -f -r {} +;
	COMMAND rm -r \"${PROJECT_BINARY_DIR}/\"
	)

#Checks for assignment and group parameter
add_custom_command(OUTPUT submit_check_param
	#Check for assignment parameter
	COMMAND if [ '$(assignment)' = '' ]\; then
		printf \"\\n\\n\\nSYNATX: make submit assignment=<1od.2> group=<group number, upper case>\\n\\n\\n\"\;
		printf \"assignment not specified\\n\\n\\n\"\;
		exit 1\;
	fi\;

	COMMAND if [ '$(group)' = '' ]\; then
		printf \"\\n\\n\\nSYNATX: make submit assignment=<1od.2> group=<group number, upper case>\\n\\n\\n\"\;
		printf \"group not specified\\n\\n\\n\"\;
		exit 1\;
	fi\;
	COMMENT "Checking for assignment and group parameter"
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	)


#Build submit archive from dir
add_custom_target(submit-nogit
	DEPENDS submit_check_param
	COMMAND printf \"\"\;
	SUBMIT_FILE=./IMA$(assignment)GR$(group).tar.bz2\;
	find . -name \"*~\" -or -iname \"*.bak\" -or -iname \"*.backup\" -or -iname \"*.swp\" -or -iname \"*.swap\" -or -iname \"*.sav\" -exec rm {} + \;
	tar cjf \"`echo $$SUBMIT_FILE`\" --exclude \"CMakeCache.txt\" --exclude \"CMakeFiles\" --exclude \"cmake_install.cmake\" --exclude \"CVS\" --exclude \"\\.svn\" --exclude \"\\.git\" --exclude \"\\./images\" --exclude \"\\./bin\" --exclude \"\\./utils\" --exclude \"\\.[^/]*\" --exclude \"*.orig\" --exclude \"*.rej\" --exclude \"*.prj\" --exclude \"\\./IMA[^/]*\\.tar\\.bz2\"  ./ \;
	printf \"\\n**********************************************************\\n\"\;
	printf \"Created: $$SUBMIT_FILE\\n\" \;
	printf \"Please Test with: tar tjfv \\\"$$SUBMIT_FILE\\\"  |less\\n\"\;
	printf \"**********************************************************\\n\\n\"\;
	if [ `ls -s -k \"$$SUBMIT_FILE\" | cut -f1 -d' '` -gt 600 ]\; then
	printf \"\\nWARNING: The tar file created is unusually large !\\n\"\;
	printf \"WARNING: make sure that you don't have unnecessary junk in your Repository!\\n\\n\\n\"\;
	fi\;
	COMMENT "Building submit archive (nogit)"
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	)


#Forces generates bochs configuration
add_custom_target(bochsconfig

	#Extracts Bochsversion from bochs --help
	COMMAND ${BOCHS_PATH} --help 2>&1 |
	grep -A 1 -m 1 \"=======\" |
	grep -o -E \"\([[:alnum:]]|\\.\)+[[:space:]]*$$\" > bochs_version\;

	IFS=.\; set -- $$\(echo \"`cat bochs_version`\"\)\;

	printf \"\\nYour bochs version is \\\"$$1.$$2.$$3\\\"\\n\\n\"\;

	major=\"$$1\"\;
	minor=\"$$2\"\;
	rev=\"$$3\"\;

	#echo \"Major: $$major Minor: $$minor Rev: $$rev\"\;

	#Empty revision string?
	if [ \"$$rev\" = \"\" ]\; then
		rev=\"0\"\;
	fi\;

	#preXXX revision string?
	if [ \"`expr substr $$rev 1 3`\" = \"pre\" ]\; then
		length=\"`expr length $$rev`\"\;
		length=\"`expr $$length - 3`\"\;
		rev=\"-`expr substr $$rev 4 $$length`\"\;
	fi\;


	#copy pattern file to real file
	cp ${PROJECT_SOURCE_DIR}/utils/bochs/bochsrc.pattern ${PROJECT_SOURCE_DIR}/utils/bochs/bochsrc\;


	#major version < 2?
	if [ \"$$major\" -lt \"2\" ] ||
	#major version == 2 AND minor version < 2
	\([ \"$$major\" -eq \"2\" ] && [ \"$$minor\" -lt \"2\" ]\) ||
	#major version == 2 AND minor version == 2 AND rev <= 0
	\([ \"$$major\" -eq \"2\" ] && [ \"$$minor\" -eq \"2\" ] && [ \"$$rev\" -le \"1\"  ]\)\; then
		#Generate bochs config for bochs version <= 2.2
		printf  \"\\nGenerating bochs config for bochs version <= 2.2.1\\n\\n\"\;
		#replace $[ROMIMAGE] with file=$$BXSHARE\\/BIOS-bochs-latest, address=0xf0000
		sed -i -e 's/\\$$\\[ROMIMAGE\\]$$/file=$$BXSHARE\\/BIOS-bochs-latest, address=0xf0000/' ${PROJECT_SOURCE_DIR}/utils/bochs/bochsrc\;
	else
		#Generate bochs config for bochs version > 2.2
		printf \"\\nGenerating bochs config for bochs version > 2.2.1\\n\\n\"\;
		#replace $[ROMIMAGE] with file=$$BXSHARE\\/BIOS-bochs-latest, address=0xf0000
		sed -i -e 's/\\$$\\[ROMIMAGE\\]$$/file=$$BXSHARE\\/BIOS-bochs-latest/' ${PROJECT_SOURCE_DIR}/utils/bochs/bochsrc\;
	fi\;

	if [ \"$$major\" -eq \"2\" ] && [ \"$$minor\" -lt \"4\" ] \; then
		#replace $[CPU] with ips: 10000000
		sed -i -e 's/\\$$\\[CPU\\]$$/ips: 10000000/' ${PROJECT_SOURCE_DIR}/utils/bochs/bochsrc\;
	else
		#Generate bochs config for bochs version > 2.4
		printf \"\\nAdapting bochs config for bochs version > 2.4\\n\\n\"\;
		#replace $[CPU] with cpu: count=1, ips=10000000
		sed -i -e 's/\\$$\\[CPU\\]$$/cpu: count=1, ips=10000000, reset_on_triple_fault=0/' '${PROJECT_SOURCE_DIR}/utils/bochs/bochsrc'\;
		#activate port_e9_hack option
		sed -i -e 's/\#\\\(port_e9_hack: enabled=1\\\)/\\1/' ${PROJECT_SOURCE_DIR}/utils/bochs/bochsrc\;
	fi\;
	COMMAND rm bochs_version

	COMMENT Generating Bochs configuration
	)

#Regenerates all configurations
add_custom_target(config
	COMMENT Generating configuration
	)

#Adds configuration dependencies to config target
#They are not implemented as commands, because then there is no conditional calling possible
#e.g. make bochsconfig during make bochs
add_dependencies(config bochsconfig)

#Outputs some information
add_custom_target(info
	COMMAND printf  \"\\nBOCHS:\\n\" > ${INFO_FILE}
	COMMAND bochs --help 2>&1 | head -n 5 >> ${INFO_FILE}
	COMMAND printf \"\\nGCC:\\n\" >> ${INFO_FILE}
	COMMAND gcc --version >> ${INFO_FILE}
	COMMAND printf \"\\nLD:\\n\" >> ${INFO_FILE}
	COMMAND ld --version >> ${INFO_FILE}
	COMMAND printf \"\\nMD5SUMS:\\n\" >> ${INFO_FILE}
	COMMAND md5sum Makefile images/* >> ${INFO_FILE}
	COMMAND printf \"\\nMAKE:\\n\" >> ${INFO_FILE}
	COMMAND make -v >> ${INFO_FILE}
	COMMAND printf \"\\nCPU:\\n\" >> ${INFO_FILE}
	COMMAND cat /proc/cpuinfo >> ${INFO_FILE}
	COMMAND printf \"\\nKERNEL:\\n\" >> ${INFO_FILE}
	COMMAND cat /proc/version >> ${INFO_FILE}
	COMMAND printf \"\\nENVIRONMENT:\\n\" >> ${INFO_FILE}
	COMMAND env >> ${INFO_FILE}
	COMMAND cat ${INFO_FILE}
	COMMENT "Building info file"
	)

#Builds the doxygen documentation
add_custom_target(doxygen
	COMMAND cp \"${PROJECT_SOURCE_DIR}/doc/sweb.dox\" \"${PROJECT_SOURCE_DIR}/doc/sweb_cmake.dox\"
	COMMAND sed -i -e 's/\\$$\\[DOC_OUTPUT_DIR\\]$$/${DOC_DIR}/' '${PROJECT_SOURCE_DIR}/doc/sweb_cmake.dox'
	COMMAND ${DOXYGEN_EXECUTABLE} \"${PROJECT_SOURCE_DIR}/doc/sweb_cmake.dox\"
	COMMAND rm \"${PROJECT_SOURCE_DIR}/doc/sweb_cmake.dox\"
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
