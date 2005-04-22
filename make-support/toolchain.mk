# $Id: toolchain.mk,v 1.6 2005/04/22 17:21:41 nomenquis Exp $
#
# $Log: toolchain.mk,v $
# Revision 1.5  2005/04/21 21:31:24  nomenquis
# added lfb support, also we now use a different grub version
# we also now read in the grub multiboot version
#
# Revision 1.4  2005/04/20 18:19:44  nomenquis
# updated these files
#
# Revision 1.3  2005/04/20 16:53:53  nomenquis
# make sure we're compiling 32 bit code only
#
# Revision 1.2  2005/04/12 18:42:51  nomenquis
# changed a zillion of iles
#
# Revision 1.1  2005/04/12 17:43:25  nomenquis
# added make support stuff
#
#
#

#ok, renicing the build is always nice
NICE := nice -n 19 
MAKE_TEMP := make 

# no one defined any cxx? Good, use gcc
ifeq ($(SWEB_USE_CXX),)
SWEB_USE_CXX := gcc
endif

# currently only gcc works
# we should definitely add something here to force
# 32 bit generation on 64 bit build systems

ifeq ($(SWEB_USE_CXX),gcc)
CXX_TEMP := g++ -m32
CC_TEMP := gcc -m32
CXX_DEP_TEMP := g++ -m32
CC_DEP_TEMP := gcc -m32
AS_TEMP := nasm -f elf
AS_DEP_TEMP := nasm -f elf
LD_TEMP := g++ -m32
KERNEL_LD_TEMP := ld -melf_i386
AR_TEMP := ar
endif

# only c++ and c code is distccd
ifeq ($(SWEB_USE_DISTCC),1)
CXX_TEMP := distcc $(CXX_TEMP)
CC_TEMP := distcc $(CC_TEMP)
endif

# nice this sucker
ifeq ($(SWEB_RENICE_BUILD),1)
CXX_TEMP := $(NICE) $(CXX_TEMP)
CC_TEMP := $(NICE) $(CC_TEMP)
AS_TEMP := $(NICE) $(AS_TEMP)
LD_TEMP := $(NICE) $(LD_TEMP)
KERNEL_LD_TEMP := $(NICE) $(KERNEL_LD_TEMP)
AR_TEMP := $(NICE) $(AR_TEMP)
CXX_DEP_TEMP := $(NICE) $(CXX_DEP_TEMP)
CC_DEP_TEMP := $(NICE) $(CC_DEP_TEMP)
AS_DEP_TEMP := $(NICE) $(AS_DEP_TEMP)
MAKE_TEMP := $(NICE) $(MAKE_TEMP)
endif

CXX := $(CXX_TEMP)
CC := $(CC_TEMP)
AS := $(AS_TEMP)
LD := $(LD_TEMP)
KERNEL_LD := $(KERNEL_LD_TEMP)
AR := $(AR_TEMP)
CXX_DEP := $(CXX_DEP_TEMP)
CC_DEP := $(CC_DEP_TEMP)
AS_DEP := $(AS_DEP_TEMP)
MAKE := $(MAKE_TEMP)

ifeq ($(SWEB_COMPILE_USERSPACE),)
ifeq ($(SWEB_USE_CXX),gcc)
CXXFLAGS := $(CXXFLAGS) -O3 -g -Wno-deprecated -Wall -W -nostdinc -fno-builtin -nostdlib -fno-rtti -nostdinc++ -fno-exceptions
CCFLAGS := $(CCFLAGS) -O3 -g -Wall -W -nostdinc -fno-builtin
ASFLAGS := $(ASFLAGS) 
LDFLAGS := $(LDFLAGS) 
endif
endif
