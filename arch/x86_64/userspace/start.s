; (c) 2005 Bernhard Tittelbach
BITS 64

extern main
extern __syscall

global _start
_start:
	; we dont touch parameters on stack, let main deal with them (argc,argv)
	call main
	mov rdi, 0x01
	mov rsi, rax
	call __syscall
