TARGET :=
INCLUDES := ../include ../../../common/include/mm/
SUBPROJECTS := arch/arch/source common/source/kernel common/source/console common/source/mm common/source/ipc utils/mtools
SHARED_LIBS :=  common/source/console/libConsole.a arch/arch/source/libArchSpecific.a common/source/kernel/libKernel.a common/source/mm/libMM.a common/source/ipc/libIPC.a
PROJECT_ROOT := .
E2FSIMAGESOURCE := utils/e2fsimage/

include ./make-support/common.mk

all: kernel e2fsimage install 

#make kernel doesn't work yet, because there is no rule kernel in common.mk
#use just "make" instead
kernel: $(SUBPROJECTS)
ifeq ($(V),1)
	@echo "$(KERNELLDCOMMAND) $(SHARED_LIBS) -u entry -u main -T arch/arch/utils/kernel-ld-script.ld -o $(OBJECTDIR)/kernel.x -Map $(OBJECTDIR)/kernel.map"
else
	@echo "LD $(OBJECTDIR)/kernel.x"
endif
	@mkdir -p $(OBJECTDIR)
	@mkdir -p $(OBJECTDIR)/sauhaufen
	@rm -f $(OBJECTDIR)/sauhaufen/*
	@bash -c 'for lib in $(SHARED_LIBS); do cd $(OBJECTDIR)/sauhaufen && ar x $${lib};done'
	@$(KERNELLDCOMMAND) $(OBJECTDIR)/sauhaufen/* -u entry -T arch/arch/utils/kernel-ld-script.ld -o $(OBJECTDIR)/kernel.x -Map $(OBJECTDIR)/kernel.map

#make install doesn't work yet, because there is no rule install in common.mk
#use just "make" instead
install: kernel
	@echo "Starting with install"
	cp ./images/boot_new.img $(OBJECTDIR)/boot.img
	test -e $(OBJECTDIR)/boot.img || (echo ERROR boot.img nowhere found; exit 1) 
	MTOOLS_SKIP_CHECK=1 $(OBJECTDIR)/utils/mtools/mtools -c mcopy -i $(OBJECTDIR)/boot.img $(OBJECTDIR)/kernel.x ::/boot/
	@echo INSTALL: $(OBJECTDIR)/boot.img is ready
	
	@echo "Starting with install - ext2"
	cp ./images/ext2fs_grub_master.img $(OBJECTDIR)/boot_ext2.img
	cp utils/e2fsimage/e2fsimage $(BINARYDESTDIR)
	test -e $(OBJECTDIR)/boot_ext2.img || (echo ERROR boot_ext2.img nowhere found; exit 1)
	test -e !$(OBJECTDIR)/e2fstemp || (rm -r $(OBJECTDIR)/e2fstemp; echo WARNING e2fstemp alredy exists - deleting.)
	mkdir $(OBJECTDIR)/e2fstemp
	mkdir $(OBJECTDIR)/e2fstemp/boot
	mkdir $(OBJECTDIR)/e2fstemp/boot/grub
	cp ./images/menu.lst $(OBJECTDIR)/e2fstemp/boot/grub
	cp $(OBJECTDIR)/kernel.x $(OBJECTDIR)/e2fstemp/boot
	$(OBJECTDIR)/bin/e2fsimage -f $(OBJECTDIR)/boot_ext2.img -d $(OBJECTDIR)/e2fstemp -n
	@echo INSTALL: $(OBJECTDIR)/boot_ext2.img is ready

e2fsimage:	
	test -e $(E2FSIMAGESOURCE)e2fsimage || $(E2FSIMAGESOURCE)configure $(E2FSIMAGESOURCE)
	
bochs:
	echo "Going to bochs -f $(SOURECDIR)/utils/bochs/bochsrc \"floppya:1_44=boot_ext2.img,status=inserted\"" 
	cd $(OBJECTDIR) && bochs -q -f $(SOURECDIR)/utils/bochs/bochsrc "floppya:1_44=boot_ext2.img,status=inserted" 

bochsc:
	echo "Going to bochs -f $(SOURECDIR)/utils/bochs/bochsrc \"floppya:1_44=boot.img,status=inserted\"" 
	cd $(OBJECTDIR) && bochs -q -f $(SOURECDIR)/utils/bochs/bochsrc "floppya:1_44=boot.img,status=inserted" <<< c
