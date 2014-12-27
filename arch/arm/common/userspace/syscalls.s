.text

__syscall:
.global __syscall
  @ this is an ugly workaround right now.
  @ go ahead and try using r0 - r5, didn't work for me
  @ but i think someone modifies r0-r3 on context switch...
  push {r4,r5}
  ldr r4, [fp, #-12]
  ldr r5, [fp, #-8]
	svc 0
	pop {r4,r5}
	bx lr

abort:
.globl abort
  bl _exit
  b .

raise:
.globl raise
  bl _exit
  b .

__write:
.globl __write
  bl _exit
  b .

stderr:
.globl stderr
  bl _exit
  b .

fflush:
.globl fflush
  bl _exit
  b .

__fprintf_chk:
.globl __fprintf_chk
  bl _exit
  b .

__aeabi_unwind_cpp_pr0:
.globl __aeabi_unwind_cpp_pr0
  bl _exit
  b .

