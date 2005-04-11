
DEFAULT_ARCH	= x86

ARCH_PATH	= ./arch
COMMON_PATH	= ./common
OBJ_DIR	= ./obj

KERNEL		= sweb-kernel
KERNEL_PATH	= $(OBJ_DIR)

SWEB_ROOT	= $(shell pwd)
export SWEB_ROOT
export ARCH_PATH
export COMMON_PATH

vpath Makefile.% $(ARCH_PATH)/%

all: arch_$(SWEB_ARCH)


arch_% : obj common
	export SWEB_ARCH $*
	$(MAKE) -C $(ARCH_PATH)/$*
	$(LD) -o $(KERNEL_PATH)/$(KERNEL) $(shell find $(OBJ_DIR) -name "dir_module")
	strip $(KERNEL_PATH)/$(KERNEL)
	$(MAKE) install

.PHONY: common
common: obj
	$(MAKE) -C $(COMMON_PATH)

.PHONY: clean
clean:
	find -iname "*.o" -exec rm {} \; -print
	find -name ".depend" -exec rm {} \; -print
	find -type l -name "objects" -exec unlink {} \; -print
	rm -f $(KERNEL_PATH)/$(KERNEL)
	rm -Rf $(OBJ_DIR)

obj:
	mkdir obj
	find $(ARCH_PATH) $(COMMON_PATH) -type d -regex '.*/\([^C][^/]*\|C[^V][^/]*\|CV[^S][^/]*\|CVS[^/]+\)' -exec mkdir $(OBJ_DIR)/{} \; -exec ln -s $(SWEB_ROOT)/$(OBJ_DIR)/{} {}/objects \; -print

install: $(KERNEL_PATH)/$(KERNEL)
	sudo mount image/grub_disk.img image/mnt -o loop,uid=$(shell id -u)
	cp -f $(KERNEL_PATH)/$(KERNEL) image/mnt/boot/
	sudo umount image/mnt
