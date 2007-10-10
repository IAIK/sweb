PROJECT_ROOT := ../..
SOURCEDIR := $(PWD)

PROJECT_ROOT := $(shell readlink -f $(PROJECT_ROOT))
TOP_DIR := $(notdir $(PROJECT_ROOT))
PARENT_DIR := $(dir $(PROJECT_ROOT))
OBJECTDIR := $(subst $(PARENT_DIR)$(TOP_DIR),$(PARENT_DIR)$(TOP_DIR)-bin,${PWD})

OBJECTS = \
			 $(OBJECTDIR)/main.o \
			 $(OBJECTDIR)/copy.o \
			 $(OBJECTDIR)/symlink.o \
			 $(OBJECTDIR)/util.o \
			 $(OBJECTDIR)/mkdir.o \
			 $(OBJECTDIR)/dirent.o \
			 $(OBJECTDIR)/mke2fs.o \
			 $(OBJECTDIR)/inodb.o \
			 $(OBJECTDIR)/sfile.o \
			 $(OBJECTDIR)/uiddb.o \
			 $(OBJECTDIR)/uids.o \
			 $(OBJECTDIR)/passwd.o

TARGET := $(OBJECTDIR)/e2fsimage

unused_OBJ=$(OBJECTDIR)/initfs.o

all: $(OBJECTDIR) $(TARGET)

Local.mak:
	./configure

$(OBJECTDIR):
	+@[ -d $@ ] || mkdir -p $@

$(TARGET): $(OBJECTS)
	@echo $(eval sinclude Local.mak)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

$(OBJECTDIR)/%.o: $(SOURCEDIR)/%.c Makefile Local.mak e2fsimage.h
	@echo $(eval sinclude Local.mak)
	$(CC) $(CPPFLAGS) -DVER=\"$(VERSION)\" $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET) Local.mak

mrproper: clean

submit:

install:

kernel:

kernelxen:
