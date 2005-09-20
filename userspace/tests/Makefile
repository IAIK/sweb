OBJECTDIR := $(subst /sweb-testing,/sweb-testing-bin,${PWD})
SOURECDIR := $(PWD)
E2P_PATH = ../../utils/exe2pseudofs/
CFLAGS = -O0 -m32 -static -nostdinc -fno-builtin -nostdlib
OBJS = $(patsubst %.c,$(OBJECTDIR)/%.sweb,$(wildcard *.c))
PSEUDOFS_IMG = ../../../sweb-testing-bin/ramfs

INCLUDE=../libc/include/
LIBC=$(OBJECTDIR)/../libc/libc.a

all: $(LIBC) $(OBJS) $(E2P_PATH)exe2pseudofs
	cd $(OJECTDIR)
	$(E2P_PATH)exe2pseudofs $(PSEUDOFS_IMG) $(basename $(OBJS))

$(E2P_PATH)exe2pseudofs: 
	$(MAKE) -C $(E2P_PATH) exe2pseudofs

$(LIBC):
	cd ../libc
	make

.PHONY: clean
clean:
	rm -f $(E2P_PATH)exe2pseudofs $(OBJS) $(PSEUDOFS_IMG)
	make -C ../libc clean

syscalls.o:
	nasm -f elf -o syscalls.o syscalls.s

$(OBJECTDIR)/%.sweb: %.c $(LIBC)
	$(CC) $(CFLAGS) -I $(INCLUDE) -o $@ $(LIBC) $<

.PHONY: mrproper
mrproper: clean
