project(sweb)

include(MakeOptions)

cmake_minimum_required(VERSION 2.4)

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
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb -g3 -gstabs -O0  -Wno-deprecated -Wall -W -nostdinc -fno-builtin -nostdlib -fno-rtti -nostdinc++ -fno-exceptions -fno-stack-protector -Wall -Wextra -nostdlib -nostartfiles -nodefaultlibs")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb -O0  -g3 -gstabs -Wall -W -nostdinc -fno-builtin -nostdlib -fno-exceptions -fno-stack-protector -Wall -Wextra  -nostdlib -nostartfiles -nodefaultlibs")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -O0  -Wno-deprecated -Wall -W -nostdinc -fno-builtin -nostdlib -fno-rtti -nostdinc++ -fno-exceptions -fno-stack-protector -Wall -Wextra -nostdlib -nostartfiles -nodefaultlibs")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0  -g3 -Wall -W -nostdinc -fno-builtin -nostdlib -fno-exceptions -fno-stack-protector -Wall -Wextra  -nostdlib -nostartfiles -nodefaultlibs")


set(LIBRARY_FILENAMES)

#Initialize global (environment) variables
set(ENV{LIBRARY_NAMES})

#Initialize CMake output directories
set(CMAKE_BINARY_DIR "${BIN_DIR_PREFIX}${BIN_DIR_NAME}") 
set(PROJECT_BINARY_DIR "${PROJECT_BINARY_DIR}/${BIN_DIR_PREFIX}${BIN_DIR_NAME}")
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

#Custom Command: Outputs kernel_image
#Executes the linker command after all libraries where build successfully
add_custom_command ( OUTPUT kernel_image
                    DEPENDS ${FINAL_LIB_NAMES} exe2minixfs
                    #DEPENDS ${FINAL_LIB_NAMES} exe2minixfs exe2pseudofs
                    COMMAND ${LD_EXECUTABLE} ${LD_ARGUMENT}
                    WORKING_DIRECTORY ${LIBRARY_OUTPUT_PATH}
                    )

#Custom Target: kernel
#Generates kernel target, which is always executed
add_custom_target ( kernel ALL
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
		    COMMAND echo ${PROJECT_BINARY_DIR}
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

#Custom target: make bochs
#Run bochs in non debugging mode
add_custom_target(bochs
                  COMMAND ${BOCHS_PATH} -q -f '${PROJECT_SOURCE_DIR}/utils/bochs/bochsrc'
                  COMMENT "Going to ${BOCHS_PATH} -f ${PROJECT_SOURCE_DIR}/utils/bochs/bochsrc"
                  WORKING_DIRECTORY ${PROJECT_BINARY_DIR} 
                  )

#Custom target: make bochsgdb
#Run bochs in debugging mode
add_custom_target(bochsgdb
                  COMMAND ${BOCHS_PATH} -q -f '${PROJECT_SOURCE_DIR}/utils/bochs/bochsrc' "gdbstub: enabled=1, port=1234"
                  COMMENT "Going to ${BOCHS_PATH} -f ${PROJECT_SOURCE_DIR}/utils/bochs/bochsrc \"gdbstub: enabled=1, port=1234\""
                  WORKING_DIRECTORY ${PROJECT_BINARY_DIR} 
                  )

#Custom target: make qemu
#Run qemu in non debugging mode
add_custom_target(qemu
                  COMMAND 	qemu -hda SWEB-flat.vmdk
                  COMMENT "Going to qemu -hda SWEB-flat.vmdk"
                  WORKING_DIRECTORY ${PROJECT_BINARY_DIR} 
                  )

#Custom target: make qemu
#Run qemu in non debugging mode
add_custom_target(qemugdb
                  COMMAND 	qemu -s -S -hda SWEB-flat.vmdk
                  COMMENT "Going to gdb qemu -s -S -hda SWEB-flat.vmdk on localhost:1234"
                  WORKING_DIRECTORY ${PROJECT_BINARY_DIR} 
                  )

#Custom target: make runddd
#Run ddd with some presets
add_custom_target(runddd
                  COMMAND ddd -cd ${PROJECT_SOURCE_DIR}/ -command ${PROJECT_SOURCE_DIR}/utils/gdb/gdbinit ${PROJECT_BINARY_DIR}/kernel.x
                  COMMENT "Going to ddd"
                  WORKING_DIRECTORY ${PROJECT_BINARY_DIR} 
		)

#Custom target: make rungdb
#Run gdb with some presets
add_custom_target(rungdb
                  COMMAND gdb -cd ${PROJECT_SOURCE_DIR}/ -command ${PROJECT_SOURCE_DIR}/utils/gdb/gdbinit ${PROJECT_BINARY_DIR}/kernel.x
                  COMMENT "Going to ddd"
                  WORKING_DIRECTORY ${PROJECT_BINARY_DIR} 
                  )

#Custom target: make mrproper
#Makes really clean
add_custom_target(mrproper
		  COMMAND make clean;
		  COMMAND find -name 'CMakeCache.txt' -exec rm {} +;
		  COMMAND find -name 'CMakeFiles' -exec rm -f -r {} +;
		  COMMAND find -name 'cmake_install.cmake' -exec rm -f -r {} +;
 		  COMMAND rm -r \"${PROJECT_BINARY_DIR}/\"
		  COMMAND cmake .
		 )

#Enables relocation of the source files (moving from /home/user1/sweb -> /home/user1/foo/.. )
add_custom_target(remove_cmake
		  COMMAND make clean;
		  COMMAND find -name 'CMakeCache.txt' -exec rm {} +;
		  COMMAND find -name 'CMakeFiles' -exec rm -f -r {} +;
		  COMMAND find -name 'cmake_install.cmake' -exec rm -f -r {} +;
 		  COMMAND rm -r \"${PROJECT_BINARY_DIR}/\"
		)

#Checks for assignment and group parameter
add_custom_command(OUTPUT submit_check_param
		#Check for assignment parameter
		COMMAND if [ '$(assignment)' = '' ]\; then
				echo \"\\n\\n\\nSYNATX: make submit assignment=<1od.2> group=<group number, upper case>\\n\\n\"\;
				echo \"assignment not specified\\n\\n\"\;
				exit 1\;
			fi\;

		COMMAND if [ '$(group)' = '' ]\; then
				echo \"\\n\\n\\nSYNATX: make submit assignment=<1od.2> group=<group number, upper case>\\n\\n\"\;
				echo \"group not specified\\n\\n\"\;
				exit 1\;
			fi\;
		COMMENT "Checking for assignment and group parameter"
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
		)


#Checks for clean hg
add_custom_command(OUTPUT submit_check_hg
		DEPENDS submit_check_param
		#Check for modified uncommited files
		COMMAND if [ `hg status -m -a -r -X images/ -X utils/ -X bin/ | wc -l` -ne 0 ]\; then
			echo \"\\n\\nWARNING: you have modified files in your working directory\"\;
			echo \"WARNING: are you sure you don't want to submit your latest changes ?\"\;
			echo \"WARNING: maybe you forgot \"hg commit\" ?\\n\\n\"\;
			fi\;

		COMMAND if [ `hg status -u | grep -i -E \"\(CMakeLists.txt|\\.c|\\.cc|\\.h|\\.cpp|\\.s|\\.tcpp\)$$\" | wc -l` -ne 0 ]\; then
			echo \"\\n\\nWARNING: you have untracked source files in your working direcory !\"\;
			echo \"WARNING: are you sure you don't want to submit those ?\"\;
			echo \"WARNING: maybe you forgot \"hg status\" and then \"hg add\" ?\\n\\n\"\;
			fi\;
		COMMENT "Checking for clean hg"
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
		)

#Build submit archive from hg tip
add_custom_target(submit
		DEPENDS submit_check_param submit_check_hg
		COMMAND SUBMIT_FILE=./IMA$(assignment)GR$(group).tar.bz2\;
			hg archive -r tip -t tbz2  -X \"utils/\" -X \"images/\" -X \"bin/\" \"`echo $$SUBMIT_FILE`\" \;
			echo \"\\n**********************************************************\"\;
			echo \"Created: $$SUBMIT_FILE\" \;
			echo \"Please Test with: tar tjfv \"$$SUBMIT_FILE\"  |less\"\;
			echo \"Make sure you didn't forget to 'hg add' new files !\"\;
			echo \"**********************************************************\\n\"\;
			if [ `ls -s -k \"$$SUBMIT_FILE\" | cut -f1 -d' '` -gt 600 ]\; then
				echo \"\\nWARNING: The tar file created is unusually large !\"\;
				echo \"WARNING: make sure that you don't have unnecessary junk in your Repository!\\n\\n\"\;
			fi\;
		COMMENT "Building submit archive (hg)"
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

#Build submit archive from dir
add_custom_target(submit-nohg
		DEPENDS submit_check_param
		COMMAND SUBMIT_FILE=./IMA$(assignment)GR$(group).tar.bz2\;
			find -name \"*~\" -or -iname \"*.bak\" -or -iname \"*.backup\" -or -iname \"*.swp\" -or -iname \"*.swap\" -or -iname \"*.sav\" -exec rm {} + \;
			tar cjf \"`echo $$SUBMIT_FILE`\" --exclude \"CMakeCache.txt\" --exclude \"CMakeFiles\" --exclude \"cmake_install.cmake\" --exclude \"CVS\" --exclude \"\\.svn\" --exclude \"\\.hg\" --exclude \"\\./images\" --exclude \"\\./bin\" --exclude \"\\./utils\" --exclude \"\\.[^/]*\" --exclude \"*.orig\" --exclude \"*.rej\" --exclude \"*.prj\" --exclude \"\\./IMA[^/]*\\.tar\\.bz2\"  ./ \;
			echo \"\\n**********************************************************\"\;
			echo \"Created: $$SUBMIT_FILE\" \;
			echo \"Please Test with: tar tjfv \"$$SUBMIT_FILE\"  |less\"\;
			echo \"**********************************************************\\n\"\;
			if [ `ls -s -k \"$$SUBMIT_FILE\" | cut -f1 -d' '` -gt 600 ]\; then
				echo \"\\nWARNING: The tar file created is unusually large !\"\;
				echo \"WARNING: make sure that you don't have unnecessary junk in your Repository!\\n\\n\"\;
			fi\;
		COMMENT "Building submit archive (nohg)"
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

#Outputs some information
add_custom_target(info
		COMMAND echo -e \"\\nBOCHS:\" > ${INFO_FILE}
		COMMAND bochs --help 2>&1 | head -n 5 >> ${INFO_FILE}
		COMMAND echo -e \"\\nGCC:\" >> ${INFO_FILE}
		COMMAND gcc --version >> ${INFO_FILE}
		COMMAND echo -e \"\\nLD:\" >> ${INFO_FILE}
		COMMAND ld --version >> ${INFO_FILE}
		COMMAND echo -e \"\\nMD5SUMS:\" >> ${INFO_FILE}
		COMMAND md5sum Makefile images/* >> ${INFO_FILE}
		COMMAND echo -e \"\\nMAKE:\" >> ${INFO_FILE}
		COMMAND make -v >> ${INFO_FILE}
		COMMAND echo -e \"\\nCPU:\" >> ${INFO_FILE}
		COMMAND cat /proc/cpuinfo >> ${INFO_FILE}
		COMMAND echo -e \"\\nKERNEL:\" >> ${INFO_FILE}
		COMMAND cat /proc/version >> ${INFO_FILE}
		COMMAND echo -e \"\\nENVIRONMENT:\" >> ${INFO_FILE}
		COMMAND env >> ${INFO_FILE}
		COMMAND cat ${INFO_FILE}
		COMMENT "Building info file"
		)

#Builds the doxygen documentation
add_custom_target(doxygen
		COMMAND cp \"${PROJECT_SOURCE_DIR}/doc/sweb.dox\" \"${PROJECT_SOURCE_DIR}/doc/sweb_cmake.dox\" 
		COMMAND sed -i 's/\\$$\\[DOC_OUTPUT_DIR\\]$$/${DOC_DIR}/' '${PROJECT_SOURCE_DIR}/doc/sweb_cmake.dox'
		COMMAND ${DOXYGEN_EXECUTABLE} \"${PROJECT_SOURCE_DIR}/doc/sweb_cmake.dox\"
		COMMAND rm \"${PROJECT_SOURCE_DIR}/doc/sweb_cmake.dox\" 
		COMMENT "Building SWEB documentation"
		)

#add_custom_target(config
#		COMMAND 

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
	
		COMMAND printf \"\\nrungdb\\t\\truns gdb, connecting to localhost:1234\\n\"

		COMMAND printf \"\\nsubmit assignment=<1 or 2> group=<group number, upper case>\\n\\t\\tBuilds a submit package using the last commited \(hg\)\\n\\t\\tversion. Uncommited changes are ignored.\\n\"

		COMMAND printf \"\\nsubmit-nohg assignment=<1 or 2> group=<group number, upper case>\\n\\t\\tBuilds a submit package without \\n\\t\\tworrying about hg\\n\"

		COMMAND printf \"\\ninfo\\t\\toutputs some important system informations\\n\"

		COMMAND printf \"\\ndoxygen\\t\\tbuilds the html and latex source documentation of SWEB\\n\"

		COMMAND printf \"\\nhelp\\t\\tshows this target summary\\n\"

		COMMAND printf \"\\n\\n\"
		)
