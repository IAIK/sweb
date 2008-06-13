project(sweb)

#Architecture to use: x86, xen
SET (ARCH x86)


set (SOURCE_WILDCARDS *.cpp *.c *.tcpp *.S)
set (ASM_WILDCARDS *.s)
set (INFO_FILE info.file)

#Find program executables needed during compilation
find_program(NASM_EXECUTABLE nasm)
find_program(LD_EXECUTABLE ld)
find_program(AR_EXECUTABLE ar)

#Comment in to enable verbose compile output
#set(CMAKE_VERBOSE_MAKEFILE true) 

#Set Compiler and compiterflags
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_C_COMPILER gcc)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb -g3 -gstabs -O0  -Wno-deprecated -Wall -W -nostdinc -fno-builtin -nostdlib -fno-rtti -nostdinc++ -fno-exceptions -fno-stack-protector -Wall -Wextra -nostdlib -nostartfiles -nodefaultlibs")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb -O0  -g3 -gstabs -Wall -W -nostdinc -fno-builtin -nostdlib -fno-exceptions -fno-stack-protector -Wall -Wextra  -nostdlib -nostartfiles -nodefaultlibs")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -O0  -Wno-deprecated -Wall -W -nostdinc -fno-builtin -nostdlib -fno-rtti -nostdinc++ -fno-exceptions -fno-stack-protector -Wall -Wextra -nostdlib -nostartfiles -nodefaultlibs")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0  -g3 -Wall -W -nostdinc -fno-builtin -nostdlib -fno-exceptions -fno-stack-protector -Wall -Wextra  -nostdlib -nostartfiles -nodefaultlibs")


set(LIBRARY_FILENAMES)

#Initialize global (environment) variables
set(ENV{LIBRARY_NAMES})

#Initialize CMake output directories
set(CMAKE_BINARY_DIR "bin") 
set(PROJECT_BINARY_DIR "${PROJECT_BINARY_DIR}/bin")
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
set(LD_ARGUMENT ${LD_ARGUMENT} -g -u entry -T ../../arch/x86/utils/kernel-ld-script.ld)
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
                    DEPENDS ${FINAL_LIB_NAMES} exe2minixfs exe2pseudofs
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
                    COMMAND test -e "./ramfs" || cp "../images/ramfs" "./ramfs"
                    COMMAND cp "./kernel.x" "./temp/boot"
                    COMMAND cp "./ramfs" "./temp/boot"
                    
                    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
                    COMMENT "Creating temp dir..."
                    )
 
 #Custom Command: Outputs empty ext2 disk image                   
 add_custom_command (OUTPUT hdd_image
                     DEPENDS temp_dir
                     COMMAND if [ ! -f ./SWEB-flat.vmdk ]; then cp "../images/SWEB-flat.vmdk.gz" "." \; gzip -df "./SWEB-flat.vmdk.gz" \; fi
                     WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
                     COMMENT "Unpacking disk image..."
                     )
                     
                    
#Custom Command: Writes helper files to temp file structure
add_custom_command (OUTPUT helper_files
                    DEPENDS hdd_image
		    COMMAND echo ${PROJECT_BINARY_DIR}
                    COMMAND cp "../images/menu.lst.hda" "./temp/boot/grub/menu.lst"
                    COMMAND	cp "../images/SWEB.vmdk" "./"
	            COMMAND cp "../images/sweb.vmx" "./"
                    #COMMAND	cp "../images/SWEB-minix.vmdk" "./"
                    #COMMAND cp "../images/SWEB-flat-minix.vmdk.gz" "./" 
	            #COMMAND gzip -df "./SWEB-flat-minix.vmdk.gz"
                    COMMAND cp "../images/nvram" "./"                   
                    
                    
                    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
                    COMMENT "Copying helper files..."
                    )
                    
#Custom Command: invoke exe2minixfs and copy the temp dir to our hd image 
add_custom_command (OUTPUT copy_to_image
	                DEPENDS helper_files exe2minixfs
	                COMMAND ./exe2minixfs SWEB-flat.vmdk 32256 ./kernel.x /boot/kernel.x ./temp/boot/ramfs /boot/ramfs ./temp/boot/grub/menu.lst /boot/grub/menu.lst
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
                  COMMAND 	bochs -q -f ../utils/bochs/bochsrc "floppya:1_44=boot_ext2.img,status=inserted"
                  COMMENT "Going to bochs -f ${PROJECT_SOURCE_DIR}/utils/bochs/bochsrc \"floppya:1_44=boot_ext2.img,status=inserted\""
                  WORKING_DIRECTORY ${PROJECT_BINARY_DIR} 
                  )

#Custom target: make bochsgdb
#Run bochs in debugging mode
add_custom_target(bochsgdb
                  COMMAND 	/opt/bochs/bin/bochs -q -f ../utils/bochs/bochsrc "floppya:1_44=boot_ext2.img,status=inserted" "gdbstub: enabled=1, port=1234"
                  COMMENT "Going to bochs -f ${PROJECT_SOURCE_DIR}/utils/bochs/bochsrc \"floppya:1_44=boot_ext2.img,status=inserted\""
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
                  COMMAND ddd -cd ${PROJECT_BINARY_DIR}/../ -command ${PROJECT_BINARY_DIR}/../utils/gdb/gdbinit ${PROJECT_BINARY_DIR}/kernel.x
                  COMMENT "Going to ddd"
                  WORKING_DIRECTORY ${PROJECT_BINARY_DIR} 
		)

#Custom target: make rungdb
#Run gdb with some presets
add_custom_target(rungdb
                  COMMAND gdb -cd ${PROJECT_BINARY_DIR}/../ -command ${PROJECT_BINARY_DIR}/../utils/gdb/gdbinit ${PROJECT_BINARY_DIR}/kernel.x
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
 		  COMMAND rm -r bin/*
		  COMMAND cmake .
		 )

#Enables relocation of the source files (moving from /home/user1/sweb -> /home/user1/foo/.. )
add_custom_target(remove_cmake
		  COMMAND make clean;
		  COMMAND find -name 'CMakeCache.txt' -exec rm {} +;
		  COMMAND find -name 'CMakeFiles' -exec rm -f -r {} +;
		  COMMAND find -name 'cmake_install.cmake' -exec rm -f -r {} +;
 		  COMMAND rm -r bin/*
		)

#TODO
add_custom_target(submit
		COMMAND echo $(assignment) $(group)
)

#TODO
add_custom_target(submit-nohg
		COMMAND echo $(assignment) $(group)
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
