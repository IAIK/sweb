E2P_PATH = ../../utils/exe2pseudofs/
CFLAGS = -O0 -m32 -static -nostdinc -fno-builtin -nostdlib
LIBS=syscalls.o start.o
OBJS = $(patsubst %.c,%.sweb,$(filter-out start.c ,$(wildcard *.c)))
PSEUDOFS_IMG = ../../../sweb-testing-bin/ramfs

all: syscalls.o start.o $(OBJS) $(E2P_PATH)exe2pseudofs
	$(E2P_PATH)exe2pseudofs $(PSEUDOFS_IMG) $(OBJS)

$(E2P_PATH)exe2pseudofs: 
	$(MAKE) -C $(E2P_PATH) exe2pseudofs

.PHONY: clean
clean:
	rm -f $(E2P_PATH)exe2pseudofs $(LIBS) $(OBJS) $(PSEUDOFS_IMG)

syscalls.o:
	nasm -f elf -o syscalls.o syscalls.s

start.o:
	$(CC) $(CFLAGS) -c start.c

%.sweb: %.c $(LIBS)
	$(CC) $(CFLAGS) -o $@ $<	$(LIBS)

.PHONY: mrproper
mrproper: clean
