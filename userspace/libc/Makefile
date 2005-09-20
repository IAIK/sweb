# Projectname: SWEB
# Simple operating system for educational purposes
#
# Copyright (C) 2005  Andreas Niederl
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.



#
# CVS Log Info for $RCSfile: Makefile,v $
#
# $Id: Makefile,v 1.4 2005/09/20 13:28:14 btittelbach Exp $
# $Log: Makefile,v $
# Revision 1.3  2005/09/20 11:23:16  btittelbach
# +_start
#
# Revision 1.2  2005/09/20 10:53:16  btittelbach
# Objects where they belong
#
# Revision 1.1  2005/09/16 05:01:40  aniederl
# import of libc Makefiles
#
#

OBJECTDIR := $(subst /sweb-testing,/sweb-testing-bin,${PWD})
SOURECDIR := $(PWD)/src

LIBC_STATIC = $(OBJECTDIR)/libc.a

INCLUDE_PATH = ./include

CFLAGS = -Wall -g -fno-builtin -nostdinc -fno-common -I $(INCLUDE_PATH) -I $(INCLUDE_PATH)/arch/arch -I $(INCLUDE_PATH)/arch/common

OBJECTS = \
					$(OBJECTDIR)/atoi.o \
					$(OBJECTDIR)/exec.o \
					$(OBJECTDIR)/file_alloc.o \
					$(OBJECTDIR)/printf.o \
					$(OBJECTDIR)/read.o \
					$(OBJECTDIR)/scanf.o \
					$(OBJECTDIR)/write.o \
					$(OBJECTDIR)/start.o


all : $(OBJECTDIR) $(LIBC_STATIC)

$(OBJECTDIR)/%.o : src/%.c
	$(CC) $(CFLAGS) -o $@ -c $^

$(OBJECTDIR)/%.o : src/%.s
	nasm -f elf -o $@ $^

$(LIBC_STATIC) : $(OBJECTS)
	ar rcs $@ $^

.PHONY: clean
.PHONY: mrproper
mrproper: clean
clean:
	rm -f ./include/arch/arch
	test -e $(OBJECTDIR) && rm -f $(OBJECTDIR)/*

$(OBJECTDIR):
	mkdir -p $(OBJECTDIR)

archlink:
ifeq ($(ARCH),amd64)
	@rm ./include/arch/arch -f
	@echo "ln -s amd64 arch"
	@cd ./include/arch/; ln -s  arch
else
	@rm ./include/arch/arch -f
	@echo "ln -s x86 arch"
	@cd ./include/arch/; ln -s x86 arch
endif

	