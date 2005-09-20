; (c) 2005 Bernhard Tittelbach

extern __syscall_exit
extern main

global _start
_start:
	; we dont touch parameters on stack, let main deal with them (argc,argv)
	call main
	; return value (exit code) from main should be in eax
	call __syscall_exit
