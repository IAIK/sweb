
#
# CVS Log Info for $RCSfile: Makefile,v $
#
# $Id: Makefile,v 1.1 2005/09/02 17:59:26 davrieb Exp $
# $Log: Makefile,v $
# Revision 1.1  2005/07/07 12:31:48  davrieb
# add ramfs
#
# Revision 1.2  2005/06/01 09:20:36  davrieb
# add all changes to fs
#
# Revision 1.1  2005/05/10 16:33:06  davrieb
# add Makefile for fs
#
#

V = 1
TARGET := fs_test_a
MAKEFLAGS :=
INCLUDES = ../../../include
PROJECT_ROOT = ../../../..
COMMON_FLAGS = -DSTANDALONE

SHARED_LIBS :=  \
                ../libFS.a \
                ..//ramfs/libRamFS.a

#all: sub

include ../../../../make-support/common.mk
MAKEFLAGS :=


sub:
	make -C .. -f Makefile.standalone
	make -C ../ramfs -f Makefile.standalone

.PHONY: sub

# all : 
# 
# fs_test_a:
# 	$(CXX) -I ../../../include -DSTANDALONE fs_test_a.o $(OBJECTDIR)/../libFS.a -o fs_test_a
