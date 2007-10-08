# vi: set sw=4 ts=4:
#
# $Id: Makefile,v 1.7 2006/10/24 19:09:48 btittelbach Exp $
#
######################################################################


all: e2fsimage

OBJS=main.o copy.o symlink.o util.o mkdir.o \
	dirent.o mke2fs.o inodb.o sfile.o uiddb.o uids.o passwd.o

unused_OBJ=initfs.o

Local.mak:
	./configure

e2fsimage: $(OBJS) 
	@echo $(eval sinclude Local.mak)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS) 

%.o: %.c Makefile Local.mak e2fsimage.h
	@echo $(eval sinclude Local.mak)
	$(CC) $(CPPFLAGS) -DVER=\"$(VERSION)\" $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) e2fsimage Local.mak 

mrproper: clean

submit:

install:

kernel:

kernelxen:
