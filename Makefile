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
               common/source/fs/pseudofs 
#               utils/mtools

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
               userspace/libc \
               userspace/tests
#               utils/mtools
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
                common/source/fs/pseudofs/libPseudoFS.a
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
                common/source/fs/pseudofs/libPseudoFS.a
endif



PROJECT_ROOT := .
E2FSIMAGESOURCE := utils/e2fsimage/



include ./make-support/common.mk


all: $(DEPS)

#make kernel doesn't work yet, because there is no rule kernel in common.mk
#use just "make" instead
kernel: $(SUBPROJECTS)
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
	@rm $(OBJECTDIR)/sauhaufen/head.o -f
	@rm $(OBJECTDIR)/sauhaufen/kprintf.o -f
#	@rm $(OBJECTDIR)/sauhaufen/Loader.o -f
#	@rm $(OBJECTDIR)/sauhaufen/Thread.o -f
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
	@rm $(PROJECT_ROOT)/arch/arch -f
	@echo "ln -s xen arch"
	@cd $(PROJECT_ROOT)/arch; ln -s xen arch
	@cd $(PROJECT_ROOT)/arch/xen/include; ln ../x86/include/*h ./ -sf
else
	@rm $(PROJECT_ROOT)/arch/arch -f
	@echo "ln -s x86 arch"
	@cd $(PROJECT_ROOT)/arch; ln -s x86 arch
endif

#make install doesn't work yet, because there is no rule install in common.mk
#use just "make" instead
install: kernel
#	@echo "Starting with install"
#	cp ./images/boot_new.img $(OBJECTDIR)/boot.img
#	test -e $(OBJECTDIR)/boot.img || (echo ERROR boot.img nowhere found; exit 1) 
#	MTOOLS_SKIP_CHECK=1 $(OBJECTDIR)/utils/mtools/mtools -c mcopy -i $(OBJECTDIR)/boot.img $(OBJECTDIR)/kernel.x ::/boot/
#	@echo INSTALL: $(OBJECTDIR)/boot.img is ready
	@echo "Starting with install - ext2 floppy"
	cp ./images/ext2fs_grub_master.img $(OBJECTDIR)/boot_ext2.img
	test -d $(OBJECTDIR)/bin || mkdir $(OBJECTDIR)/bin
	@echo "copying ef2s binary"
	cp utils/e2fsimage/e2fsimage $(OBJECTDIR)/bin/
	@echo "copying Floppy images"
	test -e $(OBJECTDIR)/disk.img || cp ./images/disk.img $(OBJECTDIR)/
	test -e $(OBJECTDIR)/boot_ext2.img || (echo ERROR boot_ext2.img nowhere found; exit 1)
	@echo "creating temp dir"
	rm -rf $(OBJECTDIR)/e2fstemp
	mkdir $(OBJECTDIR)/e2fstemp
	mkdir $(OBJECTDIR)/e2fstemp/boot
	mkdir $(OBJECTDIR)/e2fstemp/boot/grub
	cp ./images/menu.lst $(OBJECTDIR)/e2fstemp/boot/grub
	test -e $(OBJECTDIR)/ramfs || cp ./images/ramfs $(OBJECTDIR)/ramfs
	cp $(OBJECTDIR)/kernel.x $(OBJECTDIR)/e2fstemp/boot
	cp $(OBJECTDIR)/ramfs $(OBJECTDIR)/e2fstemp/boot
	$(OBJECTDIR)/bin/e2fsimage -f $(OBJECTDIR)/boot_ext2.img -d $(OBJECTDIR)/e2fstemp -n
	@echo "########## $(OBJECTDIR)/boot_ext2.img is ready ###########"
	@echo "########## Starting with install - ext2 hard drive ###########"
	@echo $(OBJECTDIR)/SWEB-flat.vmdk
	! ( test -e "$(OBJECTDIR)/SWEB-flat.vmdk" ) || echo "SWEB-flat.vmdk does exist. using it..."
	test -e "$(OBJECTDIR)/SWEB-flat.vmdk" || echo "SWEB-flat.vmdk does not exist. creating it..."
	test -e "$(OBJECTDIR)/SWEB-flat.vmdk" || ( cp ./images/SWEB-flat.vmdk.gz $(OBJECTDIR)/ ; gzip -df $(OBJECTDIR)/SWEB-flat.vmdk.gz )
	@echo "copying helper files..."
	cp ./images/menu.lst.hda $(OBJECTDIR)/e2fstemp/boot/grub/menu.lst
	cp ./images/SWEB.vmdk $(OBJECTDIR)/
	cp ./images/sweb.vmx $(OBJECTDIR)/
	cp ./images/nvram $(OBJECTDIR)/
	@echo "copy files to image..."
	dd if=$(OBJECTDIR)/SWEB-flat.vmdk of=$(OBJECTDIR)/temp_fs_ext2 bs=512 skip=63 2> /dev/null
	$(OBJECTDIR)/bin/e2fsimage -f $(OBJECTDIR)/temp_fs_ext2 -d $(OBJECTDIR)/e2fstemp -n
	dd of=$(OBJECTDIR)/SWEB-flat.vmdk if=$(OBJECTDIR)/temp_fs_ext2 bs=512 seek=63 2> /dev/null
	rm -f $(OBJECTDIR)/temp_fs_ext2
	@echo "########## Finished installing - ext2 hard drive ##########"

e2fsimage:	
	test -e $(E2FSIMAGESOURCE)e2fsimage || $(E2FSIMAGESOURCE)configure $(E2FSIMAGESOURCE)

qemu:
	echo "Going to run qemu -hda SWEB-flat.vmdk"
	cd $(OBJECTDIR) && qemu -hda SWEB-flat.vmdk

vmware:
	echo "Going to run \"vmware start $(OBJECTDIR)/sweb.vmx\""
	cd $(OBJECTDIR) && vmrun start "$(OBJECTDIR)/sweb.vmx"

bochs:
	echo "Going to bochs -f $(SOURECDIR)/utils/bochs/bochsrc \"floppya:1_44=boot_ext2.img,status=inserted\"" 
	cd $(OBJECTDIR) && bochs -q -f $(SOURECDIR)/utils/bochs/bochsrc "floppya:1_44=boot_ext2.img,status=inserted" 
bochsc:
	echo "Going to bochs -f $(SOURECDIR)/utils/bochs/bochsrc \"floppya:1_44=boot_ext2.img,status=inserted\"" 
	cd $(OBJECTDIR) && bochs -q -f $(SOURECDIR)/utils/bochs/bochsrc "floppya:1_44=boot_ext2.img,status=inserted"<<< c

bochsgdb:
	echo "Going to gdb bochs on port localhost:1234 " 
	cd $(OBJECTDIR) && bochs -q -f $(SOURECDIR)/utils/bochs/bochsrc "floppya:1_44=boot_ext2.img,status=inserted" "gdbstub: enabled=1, port=1234"
