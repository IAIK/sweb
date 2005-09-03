OBJS = $(patsubst %.c,%,$(wildcard *.c))
CFLAGS = -m32 -static -nostdinc -fno-builtin -nostdlib

all: exe2pseudofs $(OBJS)
	./exe2pseudofs pseudofs.img $(OBJS)

exe2pseudofs: exe2pseudofs.cpp exe2pseudofs.h
	$(CXX) -m32 -W -o exe2pseudofs exe2pseudofs.cpp

.PHONY: clean
clean:
	rm -f exe2pseudofs $(OBJS)
	rm -f pseudofs.img

finite-loop2:
	gcc $(CFLAGS) -o finite-loop2  ../../userspace/start.o finite-loop2.c
