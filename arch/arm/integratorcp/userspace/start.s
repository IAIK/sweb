_start:
.globl _start
  bl main
  bl _exit

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
