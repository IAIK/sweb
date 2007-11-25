TARGET :=
INCLUDES := ../include ../../../common/include/mm/

ifeq ($(ARCH),xen)
DEPS := kernelxen
else
DEPS:= kernel e2fsimage install
endif

ifeq ($(ARCH),xen)
SUBPROJECTS := \
               arch/arch/source \
               common/source/util \
               common/source/kernel \
               common/source/console \
               common/source/ipc \
               common/source/mm \
               common/source/drivers \
               common/source/fs \
               common/source/fs/ramfs \
               common/source/fs/pseudofs \
	           utils/e2fsimage \
               common/source/fs/devicefs 

else
SUBPROJECTS := \
		arch/arch/source \
		common/source/util \
		common/source/kernel \
		common/source/console \
		common/source/ipc \
		common/source/mm \
		common/source/drivers \
		common/source/fs \
		common/source/fs/ramfs \
		common/source/fs/pseudofs \
		common/source/fs/devicefs \
		common/source/fs/minixfs \
		userspace/libc \
		utils/e2fsimage \
		userspace/tests

endif

ifeq ($(ARCH),xen)
SHARED_LIBS :=  \
                arch/arch/source/libArchSpecific.a \
                common/source/util/libUtil.a \
                common/source/kernel/libKernel.a \
                common/source/console/libConsole.a \
                common/source/ipc/libIPC.a \
                common/source/mm/libMM.a \
                common/source/drivers/libDrivers.a \
                common/source/fs/libFS.a \
                common/source/fs/ramfs/libRamFS.a \
                common/source/fs/pseudofs/libPseudoFS.a \
                common/source/fs/devicefs/libDeviceFS.a
else
SHARED_LIBS :=  \
                arch/arch/source/libArchSpecific.a \
                common/source/util/libUtil.a \
                common/source/kernel/libKernel.a \
                common/source/console/libConsole.a \
                common/source/ipc/libIPC.a \
                common/source/mm/libMM.a \
                common/source/drivers/libDrivers.a \
                common/source/fs/libFS.a \
                common/source/fs/ramfs/libRamFS.a \
                common/source/fs/pseudofs/libPseudoFS.a \
                common/source/fs/minixfs/libMinixFS.a \
                common/source/fs/devicefs/libDeviceFS.a
endif



PROJECT_ROOT := .
E2FSIMAGESOURCE := utils/e2fsimage/



include ./make-support/common.mk


all: $(DEPS)

#make kernel doesn't work yet, because there is no rule kernel in common.mk
#use just "make" instead
kernel: $(SUBPROJECTS)
	@echo "Starting with kernel"
ifeq ($(V),1)
	@echo "$(KERNELLDCOMMAND) $(SHARED_LIBS) -g -u entry -T arch/arch/utils/kernel-ld-script.ld -o $(OBJECTDIR)/kernel.x -Map $(OBJECTDIR)/kernel.map"
else
	@echo "LD $(OBJECTDIR)/kernel.x"
endif
	@mkdir -p $(OBJECTDIR)
	@mkdir -p $(OBJECTDIR)/sauhaufen
	@rm -f $(OBJECTDIR)/sauhaufen/*
	@bash -c 'for lib in $(SHARED_LIBS); do cd $(OBJECTDIR)/sauhaufen && ar x $${lib};done'
	@$(KERNELLDCOMMAND) $(OBJECTDIR)/sauhaufen/* -g -u entry -T arch/x86/utils/kernel-ld-script.ld -o $(OBJECTDIR)/kernel.x -Map $(OBJECTDIR)/kernel.map

$(SUBPROJECTS): archlink

kernelxen: $(SUBPROJECTS)
ifeq ($(V),1)
#	@echo "$(KERNELLDCOMMAND) $(OBJECTDIR)/sauhaufen/* $(SHARED_LIBS) -u _start -T arch/xen/utils/kernel-ld-script.ld -o $(OBJECTDIR)/kernel.x -Map $(OBJECTDIR)/kernel.map"
	@echo "ld -N -T arch/xen/utils/kernel-ld-script.ld $(OBJECTDIR)/arch/arch/source/head.o  $(OBJECTDIR)/sauhaufen/main.o $(OBJECTDIR)/arch/arch/source/libArchSpecific.a -g -u _start -o $(OBJECTDIR)/sweb_xen.elf -Map $(OBJECTDIR)/kernel.map"
else
	@echo "LD $(OBJECTDIR)/kernel.x"
endif
	@mkdir -p $(OBJECTDIR)
	@mkdir -p $(OBJECTDIR)/sauhaufen
	@rm -f $(OBJECTDIR)/sauhaufen/*
	@bash -c 'for lib in $(SHARED_LIBS); do cd $(OBJECTDIR)/sauhaufen && ar x $${lib};done'
	@rm -f $(OBJECTDIR)/sauhaufen/head.o
#	@rm -f $(OBJECTDIR)/sauhaufen/kprintf.o 
#	@rm -f $(OBJECTDIR)/sauhaufen/Loader.o 
#	@rm -f $(OBJECTDIR)/sauhaufen/Thread.o 
#	@ld -N -T arch/xen/utils/kernel-ld-script.ld $(OBJECTDIR)/arch/arch/source/head.o  $(OBJECTDIR)/sauhaufen/main.o  $(OBJECTDIR)/sauhaufen/arch_panic.o  $(SHARED_LIBS) -g -u _start -o $(OBJECTDIR)/sweb_xen.elf -Map $(OBJECTDIR)/kernel.map
	@ld -N -T arch/xen/utils/kernel-ld-script.ld $(OBJECTDIR)/arch/arch/source/head.o  $(OBJECTDIR)/sauhaufen/*  -g -u _start -o $(OBJECTDIR)/sweb_xen.elf -Map $(OBJECTDIR)/kernel.map
	@echo "objcopy -R .note -R .comment  $(OBJECTDIR)/sweb_xen.elf  $(OBJECTDIR)/sweb_xen.x"
	objcopy -R .note -R .comment  $(OBJECTDIR)/sweb_xen.elf  $(OBJECTDIR)/sweb_xen.x
ifeq ($(V),1)
	@echo "gzip -f -9 -c $(OBJECTDIR)/sweb_xen.x >$(OBJECTDIR)/sweb_xen.gz"
else
	@echo "GZ image is sweb_xen.gz"
endif
	gzip -f -9 -c $(OBJECTDIR)/sweb_xen.x >$(OBJECTDIR)/sweb_xen.gz

archlink:
ifeq ($(ARCH),xen)
	@rm -f $(PROJECT_ROOT)/arch/arch
	@echo "ln -s xen arch"
	@cd $(PROJECT_ROOT)/arch; ln -s xen arch
	@cd $(PROJECT_ROOT)/arch/xen/include; ln ../x86/include/*h ./ -sf
else
	@rm -f $(PROJECT_ROOT)/arch/arch 
	@echo "ln -s x86 arch"
	@cd $(PROJECT_ROOT)/arch; ln -s x86 arch
endif

#make install doesn't work yet, because there is no rule install in common.mk
#use just "make" instead
install: kernel
	@echo "Starting with install"
#	cp ./images/boot_new.img $(OBJECTDIR)/boot.img
#	test -e $(OBJECTDIR)/boot.img || (echo ERROR boot.img nowhere found; exit 1)
#	MTOOLS_SKIP_CHECK=1 $(OBJECTDIR)/utils/mtools/mtools -c mcopy -i $(OBJECTDIR)/boot.img $(OBJECTDIR)/kernel.x ::/boot/
#	@echo INSTALL: $(OBJECTDIR)/boot.img is ready
#	@echo "Starting with install - ext2 floppy"
#	cp ./images/ext2fs_grub_master.img $(OBJECTDIR)/boot_ext2.img
	test -d $(OBJECTDIR)/bin || mkdir $(OBJECTDIR)/bin
	@echo "copying ef2s binary"
	cp $(OBJECTDIR)/utils/e2fsimage/e2fsimage $(OBJECTDIR)/bin/
#	@echo "copying Floppy images"
#	test -e $(OBJECTDIR)/boot_ext2.img || (echo ERROR boot_ext2.img nowhere found; exit 1)
	@echo "creating temp dir"
	rm -rf $(OBJECTDIR)/e2fstemp
	mkdir $(OBJECTDIR)/e2fstemp
	mkdir $(OBJECTDIR)/e2fstemp/boot
	mkdir $(OBJECTDIR)/e2fstemp/boot/grub
	cp ./images/menu.lst $(OBJECTDIR)/e2fstemp/boot/grub
	test -e $(OBJECTDIR)/ramfs || cp ./images/ramfs $(OBJECTDIR)/ramfs
	cp $(OBJECTDIR)/kernel.x $(OBJECTDIR)/e2fstemp/boot
	cp $(OBJECTDIR)/ramfs $(OBJECTDIR)/e2fstemp/boot
#	$(OBJECTDIR)/bin/e2fsimage -f $(OBJECTDIR)/boot_ext2.img -d $(OBJECTDIR)/e2fstemp -n
#	@echo "########## $(OBJECTDIR)/boot_ext2.img is ready ###########"
	@echo "########## Starting with install - ext2 hard drive ###########"
	@echo $(OBJECTDIR)/SWEB-flat.vmdk
	cp ./images/SWEB-flat.vmdk.gz $(OBJECTDIR)/ ; gzip -df $(OBJECTDIR)/SWEB-flat.vmdk.gz
	@echo "copying helper files..."
	cp ./images/menu.lst.hda $(OBJECTDIR)/e2fstemp/boot/grub/menu.lst
	cp ./images/SWEB.vmdk $(OBJECTDIR)/
	cp ./images/sweb.vmx $(OBJECTDIR)/
#-- this should be done if you want to use the same minix image
	cp ./images/SWEB-minix.vmdk $(OBJECTDIR)/
	cp ./images/SWEB-flat-minix.vmdk.gz $(OBJECTDIR)/ ; gzip -df $(OBJECTDIR)/SWEB-flat-minix.vmdk.gz
#-- so uncomment this and use make minixfs
	cp ./images/nvram $(OBJECTDIR)/
	@echo "copy files to image..."
	dd if=$(OBJECTDIR)/SWEB-flat.vmdk of=$(OBJECTDIR)/temp_fs_ext2 bs=512 skip=63 2> /dev/null
	$(OBJECTDIR)/bin/e2fsimage -f $(OBJECTDIR)/temp_fs_ext2 -d $(OBJECTDIR)/e2fstemp -n
	dd of=$(OBJECTDIR)/SWEB-flat.vmdk if=$(OBJECTDIR)/temp_fs_ext2 bs=512 seek=63 2> /dev/null
	rm -f $(OBJECTDIR)/temp_fs_ext2
	@echo "########## Finished installing - ext2 hard drive ##########"


minixfs:
	dd if=/dev/zero of=./images/SWEB-flat-minix.vmdk bs=512 count=20808
	sudo mkfs.minix ./images/SWEB-flat-minix.vmdk

e2fsimage:
	@echo "Starting with e2fsimage"
	test -e $(E2FSIMAGESOURCE)e2fsimage || $(E2FSIMAGESOURCE)configure $(E2FSIMAGESOURCE)

qemu:
	echo "Going to run qemu -hda SWEB-flat.vmdk"
	cd $(OBJECTDIR) && qemu -hda SWEB-flat.vmdk

qemugdb:
	echo "Going to gdb qemu on localhost:1234 using SWEB-flat.vmdk as hda"
	cd $(OBJECTDIR) && qemu -s -S -hda SWEB-flat.vmdk

vmware:
	echo "Going to run \"vmware start $(OBJECTDIR)/sweb.vmx\""
	cd $(OBJECTDIR) && vmrun start "$(OBJECTDIR)/sweb.vmx"

bochs:
	echo "Going to bochs -f $(SOURCEDIR)/utils/bochs/bochsrc \"floppya:1_44=boot_ext2.img,status=inserted\""
	cd $(OBJECTDIR) && bochs -q -f $(SOURCEDIR)/utils/bochs/bochsrc "floppya:1_44=boot_ext2.img,status=inserted"
bochsc:
	echo "Going to bochs -f $(SOURCEDIR)/utils/bochs/bochsrc \"floppya:1_44=boot_ext2.img,status=inserted\""
	cd $(OBJECTDIR) && bochs -q -f $(SOURCEDIR)/utils/bochs/bochsrc "floppya:1_44=boot_ext2.img,status=inserted"<<< c

bochsgdb:
	echo "Going to gdb bochs on port localhost:1234 "
	cd $(OBJECTDIR) && bochs -q -f $(SOURCEDIR)/utils/bochs/bochsrc "floppya:1_44=boot_ext2.img,status=inserted" "gdbstub: enabled=1, port=1234"

.PHONY: rungdb runddd
rungdb:
	gdb -cd $(SOURCEDIR) -command $(SOURCEDIR)/utils/gdb/gdbinit $(OBJECTDIR)/kernel.x
runddd:
	ddd -cd $(SOURCEDIR) -command $(SOURCEDIR)/utils/gdb/gdbinit $(OBJECTDIR)/kernel.x

.PHONY: prepare-system-ubuntu
prepare-system-ubuntu:
	$(info PREPARE-SYSTEM: you may have to manualy choose a libstdc++-dev)
	$(info PREPARE-SYSTEM: running: sudo apt-get install e2fslibs-dev nasm mercurial bochs-x libstdc++-dev)
	@echo
	sudo apt-get install e2fslibs-dev nasm mercurial bochs-x libstdc++-dev

.PHONY: submit
SUBMIT_FILE:=./IMA$(assignment)GR$(group).tar.bz2
submit:
ifneq ($(shell hg status -m -a -r -X images/ -X utils/ -X bin/ | wc -l),0)
	$(warning )
	$(warning WARNING: you have modified files in your working directory)
	$(warning WARNING: are you sure you don't want to submit your latest changes ?)
	$(warning WARNING: maybe you forgot "hg commit" ?)
	$(warning )

endif
ifneq ($(shell hg status -u | grep -i -E "(Makefile|\.c|\.cc|\.h|\.cpp|\.s|\.tcpp)$$" | wc -l),0)
	$(warning )
	$(warning WARNING: you have untracked source files in your working direcory !)
	$(warning WARNING: are you sure you don't want to submit those ?)
	$(warning WARNING: maybe you forgot "hg status" and then "hg add" ?)
	$(warning )
endif
ifndef assignment
	$(warning )
	$(warning SYNATX: make submit assignment=<1od.2> group=<group number, upper case>)
	$(warning )
	$(error assignment not specified)
endif
ifndef group
	$(warning )
	$(warning SYNATX: make submit assignment=<1od.2> group=<group number, upper case>)
	$(warning )
	$(error group not specified)
endif
	#@$(MAKE) mrproper
	hg archive -r tip -t tbz2 -X "utils/" -X "images/" -X "bin/" "$(SUBMIT_FILE)" 
	@echo -e "\n**********************************************"
	@echo "Created: $(SUBMIT_FILE)" 
	@echo "Please Test with: tar tjfv \"$(SUBMIT_FILE)\"  |less"
	@echo "Make sure you didn't forget to 'hg add' new files !"
	@echo -e "**********************************************\n"
	@test $$( ls -s -k "$(SUBMIT_FILE)" | cut -f1 -d' ' ) -lt 600 || echo -e "\nWARNING: The tar file created is unusually large !\nWARNING: make sure that you don't have unnecessary junk in your Repository!\n\n"

INFO_FILE=info.file
.PHONY: info
info:
	echo -e "\nBOCHS:" > $(INFO_FILE)
	bochs --help 2>&1 | head -n 5 >> $(INFO_FILE)
	echo -e "\nGCC:" >> $(INFO_FILE)
	gcc --version >> $(INFO_FILE)
	echo -e "\nLD:" >> $(INFO_FILE)
	ld --version >> $(INFO_FILE)
	echo -e "\nAWK:" >> $(INFO_FILE)
	awk --version >> $(INFO_FILE)
	echo -e "\nMD5SUMS:" >> $(INFO_FILE)
	md5sum Makefile images/* >> $(INFO_FILE)
	echo -e "\nMAKE:" >> $(INFO_FILE)
	$(MAKE) -v >> $(INFO_FILE)
	echo -e "\nLIBEXT2:" >> $(INFO_FILE)
	/sbin/ldconfig -p | grep ext2 >> $(INFO_FILE)
	echo -e "\nCPU:" >> $(INFO_FILE)
	cat /proc/cpuinfo >> $(INFO_FILE)
	echo -e "\nKERNEL:" >> $(INFO_FILE)
	cat /proc/version >> $(INFO_FILE)
	echo -e "\nENVIRONMENT:" >> $(INFO_FILE)
	env >> $(INFO_FILE)
	echo -e "\nE2FSIMAGE/Local.mak:" >> $(INFO_FILE)
	ls -la utils/e2fsimage/Local.mak >> $(INFO_FILE)
	cat $(INFO_FILE)

mrproper:
	rm -rf $(OBJECTDIR)
