.cpu cortex-m4
.syntax unified
.thumb
.text



/*
* SYSTEM CALL
*/
.global __syscall
.type __syscall, %function
__syscall:
	svc 0
	bx lr
