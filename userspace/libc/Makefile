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

PROJECT_ROOT := ../..
SOURECDIR := $(PWD)/src
ARCHDIR := ../../arch/arch/userspace

PROJECT_ROOT := $(shell readlink -f $(PROJECT_ROOT))
TOP_DIR := $(notdir $(PROJECT_ROOT))
PARENT_DIR := $(dir $(PROJECT_ROOT))
OBJECTDIR := $(subst $(PARENT_DIR)$(TOP_DIR),$(PARENT_DIR)$(TOP_DIR)-bin,${PWD})

LIBC_STATIC = $(OBJECTDIR)/libc.a

INCLUDE_PATH = ./include

CFLAGS = -Wall -g -m32 -fno-builtin -nostdinc -fno-stack-protector -fno-common -I $(INCLUDE_PATH) -I $(INCLUDE_PATH)/arch/arch -I $(INCLUDE_PATH)/arch/common

OBJECTS = \
					$(OBJECTDIR)/atoi.o \
					$(OBJECTDIR)/exec.o \
					$(OBJECTDIR)/file_alloc.o \
					$(OBJECTDIR)/printf.o \
					$(OBJECTDIR)/read.o \
					$(OBJECTDIR)/scanf.o \
					$(OBJECTDIR)/write.o \
					$(OBJECTDIR)/string.o \
					$(OBJECTDIR)/ctype.o \
					$(OBJECTDIR)/start.o \
					$(OBJECTDIR)/syscalls.o


all : $(OBJECTDIR) $(LIBC_STATIC)

$(OBJECTDIR)/%.o : src/%.c
	$(CC) $(CFLAGS) -o $@ -c $^

$(OBJECTDIR)/%.o : $(ARCHDIR)/%.s
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

submit:
