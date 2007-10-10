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
