# $Id: toolchain.mk,v 1.14 2005/09/21 17:01:12 nomenquis Exp $
#
# $Log: toolchain.mk,v $
# Revision 1.13  2005/07/31 17:35:32  nightcreature
# additions for xen target
#
# Revision 1.12  2005/07/22 15:29:44  nomenquis
# added gdb support
#
# Revision 1.11  2005/05/31 13:23:25  nelles
# panic function finally works as it should (I have lowered
# the optimisation
# level
# from O2 to O1 and the problem seems to be fixed)
#
# p.s. I really hate the vi ... Notepad RULES
#
# Revision 1.10  2005/05/08 21:43:55  nelles
# changed gcc flags from -g to -g3 -gstabs in order to
# generate stabs output in object files
# changed linker script to load stabs in kernel
# in bss area so GRUB loads them automaticaly with
# the bss section
#
# changed StupidThreads in main for testing purposes
#
# Revision 1.9  2005/04/27 09:19:20  nomenquis
# only pack whats needed
#
# Revision 1.8  2005/04/26 15:58:46  nomenquis
# threads, scheduler, happy day
#
# Revision 1.7  2005/04/24 16:58:04  nomenquis
# ultra hack threading
#
# Revision 1.6  2005/04/22 17:21:41  nomenquis
# added TONS of stuff, changed ZILLIONS of things
#
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
ASGCC_TEMP := gcc -D__ASSEMBLY__ -m32
ASGCC_DEP_TEMP := gcc -D__ASSEMBLY__ -m32
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
ASGCC_TEMP := gcc -D__ASSEMBLY__ -m32
ASGCC_DEP_TEMP := gcc -D__ASSEMBLY__ -m32
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
ASGCC := $(ASGCC_TEMP)
ASGCC_DEP := $(ASGCC_DEP_TEMP)

ifeq ($(SWEB_COMPILE_USERSPACE),)
ifeq ($(SWEB_USE_CXX),gcc)
#CXXFLAGS := $(CXXFLAGS) -fpack-struct -g -O1  -Wno-deprecated -Wall -W -nostdinc -fno-builtin -nostdlib -fno-rtti -nostdinc++ -fno-exceptions
#CCFLAGS := $(CCFLAGS) -fpack-struct -g -O1  -Wall -W -nostdinc -fno-builtin
CXXFLAGS := $(CXXFLAGS) -ggdb -O0  -Wno-deprecated -Wall -W -nostdinc -fno-builtin -nostdlib -fno-rtti -nostdinc++ -fno-exceptions -fno-stack-protector
CCFLAGS := $(CCFLAGS)  -O0 -ggdb  -Wall -W -nostdinc -fno-builtin -fno-stack-protector
ASFLAGS := $(ASFLAGS) 
LDFLAGS := $(LDFLAGS) 
ASGCCFLAGS := $(ASGCCFLAGS)  -c -O0 -g3 -gstabs -Wall -W -nostdinc -fno-builtin -fno-stack-protector
endif
endif
