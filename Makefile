TARGET :=
INCLUDES := ../include ../../../common/include/mm/
SUBPROJECTS := arch/arch/source common/source/kernel
SHARED_LIBS := arch/arch/source/libArchSpecific.a common/source/kernel/libKernel.a
PROJECT_ROOT := .

include ./make-support/common.mk

all: kernel install

#make kernel doesn't work yet, because there is no rule kernel in common.mk
#use just "make" instead
kernel: $(SUBPROJECTS)
ifeq ($(V),1)
	@echo "$(LDCOMMAND) $(SHARED_LIBS) -u entry -T arch/arch/utils/kernel-ld-script.ld -o $(OBJECTDIR)/kernel"
else
	@echo "LD $(OBJECTDIR)/kernel.x"
endif
	@mkdir -p $(OBJECTDIR)
	@$(LDCOMMAND) $(SHARED_LIBS) -u entry -T arch/arch/utils/kernel-ld-script.ld -o $(OBJECTDIR)/kernel

#make install doesn't work yet, because there is no rule install in common.mk
#use just "make" instead
install: kernel
	test -e ./image/grub_disk.img && cp ./image/grub_disk.img $(OBJECTDIR)/grub_disk.img
	test -e $(OBJECTDIR)/grub_disk.img || (echo ERROR grub_disk.img nowhere found; exit 1) 
	MTOOLS_SKIP_CHECK=1 mcopy -i $(OBJECTDIR)/grub_disk.img $(OBJECTDIR)/kernel ::
	@echo INSTALL: $(OBJECTDIR)/grub_disk.img is ready

