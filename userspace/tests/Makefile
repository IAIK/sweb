

all: 
	nasm -f elf -o syscalls.o syscalls.s
	gcc mult.c syscalls.o -m32 -o mult -static -nostdinc -fno-builtin -nostdlib
