TARGET :=
INCLUDES := ../include ../../../common/include/mm/
SUBPROJECTS := arch/arch/source common/source/kernel
SHARED_LIBS := arch/arch/source/libArchSpecific.a common/source/kernel/libKernel.a
PROJECT_ROOT := .

include ./make-support/common.mk

all: kernel

kernel: $(SUBPROJECTS)
ifeq ($(V),1)
	@echo "$(LDCOMMAND) $(SHARED_LIBS) -u entry -T arch/arch/utils/kernel-ld-script.ld -o $(OBJECTDIR)/kernel"
else
	@echo "LD $(OBJECTDIR)/kernel.x"
endif
	@mkdir -p $(OBJECTDIR)
	@$(LDCOMMAND) $(SHARED_LIBS) -u entry -T arch/arch/utils/kernel-ld-script.ld -o $(OBJECTDIR)/kernel
