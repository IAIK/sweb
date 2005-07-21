OBJS = $(patsubst %.c,%,$(wildcard *.c))
CFLAGS = -static -nostdinc -fno-builtin -nostdlib

all: exe2str $(OBJS)
	./exe2str $(OBJS) > out.cstr

exe2str: exe2str.hs
	ghc -o exe2str exe2str.hs
	rm exe2str.o exe2str.hi

.PHONY: clean
clean:
	rm -f exe2str $(OBJS) *.hi out.cstr

finite-loop2:
	gcc $(CFLAGS) -o finite-loop2  ../../userspace/start.o finite-loop2.c
