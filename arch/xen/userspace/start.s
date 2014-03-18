; (c) 2005 Bernhard Tittelbach
BITS 32

extern main
extern __syscall

global _start
_start:
	; we dont touch parameters on stack, let main deal with them (argc,argv)
	call main
	push 0x00	; if we don't push stuff onto the stack prior to calling __syscall
	push 0x00	; __syscall will read beyond the start of the stack
	push 0x00	; and trigger a pagefault
	push 0x00
	push eax		; return value (exit code) from main should be in eax
	push 0x01	; syscall code for exit == 1
	call __syscall
