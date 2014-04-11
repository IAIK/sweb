.text

__syscall:
.global __syscall
  @ this is an ugly workaround right now.
  @ go ahead and try using r0 - r5, didn't work for me
  @ but i think someone modifies r0-r3 on context switch...
  push {r4,r5,r6,r7,r8,r9}
  mov r4, r0
  mov r5, r1
  mov r6, r2
  mov r7, r3
  ldr r8, [fp, #-12]
  ldr r9, [fp, #-8]
	svc 0
	pop {r4,r5,r6,r7,r8,r9}
	bx lr
