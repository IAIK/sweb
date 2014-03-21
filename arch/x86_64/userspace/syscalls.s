
BITS 64

section .text

global __syscall
__syscall:
  int 0x80
  ret
  
