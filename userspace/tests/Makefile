OBJECTDIR := $(subst /sweb-testing,/sweb-testing-bin,${PWD})
SOURECDIR := $(PWD)
E2P_PATH = ../../utils/exe2pseudofs/
CFLAGS = -O0 -m32 -static -nostdinc -fno-builtin -nostdlib
LIBS=$(OBJECTDIR)/../libc/libc.a
#$(OBJECTDIR)/syscalls.o $(OBJECTDIR)/start.o
OBJS = $(patsubst %.c,$(OBJECTDIR)/%.sweb,$(wildcard *.c))
PSEUDOFS_IMG = ../../../sweb-testing-bin/ramfs

all: $(OBJECTDIR) $(LIBS) $(OBJS) $(E2P_PATH)exe2pseudofs
	$(E2P_PATH)exe2pseudofs $(PSEUDOFS_IMG) $(OBJS)

$(E2P_PATH)exe2pseudofs: 
	$(MAKE) -C $(E2P_PATH) exe2pseudofs

$(OBJECTDIR):
	mkdir -p $(OBJECTDIR)

.PHONY: clean
clean:
	rm -f $(E2P_PATH)exe2pseudofs $(LIBS) $(OBJS) $(PSEUDOFS_IMG)

$(OBJECTDIR)/%.o: %.s
	nasm -f elf -o $@ $<

$(OBJECTDIR)/%.sweb: %.c $(LIBS)
	$(CC) $(CFLAGS) -o $@ $<	-Wl,-whole-archive $(LIBS)

.PHONY: mrproper
mrproper: clean
