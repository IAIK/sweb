/**
 * @file syscall-definitions.h
 * defines the various syscall names used by class @ref Syscall
 */

#define fd_stdin 0
#define fd_stdout 1
#define fd_stderr 2

#define sc_restart 0
#define sc_exit 1
#define sc_fork 2
#define sc_read 3
#define sc_write 4
#define sc_open 5
#define sc_close 6
#define sc_waitpid 7
#define sc_creat 8
#define sc_link 9
#define sc_unlink 10
#define sc_execve 11
//....
#define sc_lseek 19
#define sc_getpid 20
#define sc_mount 21
#define sc_umount 22
//....
#define sc_nice 34
//....
#define sc_kill 37
//....
#define sc_rename 38
#define sc_mkdir 39
#define sc_rmdir 40
#define sc_dup 41
#define sc_pipe 42
#define sc_pseudols 43
//....
#define sc_brk  45
//....
#define sc_signal 48
//....
#define sc_dup2 63
//....
#define sc_reboot 88
//....
#define sc_outline 105
//....
#define sc_ipc 117
//....
#define sc_clone 120
//....
#define sc_flock 143
#define sc_msync 144
//....
#define sc_sched_yield 158
//....
#define sc_vfork 190
#define sc_createprocess 191

#define sc_trace 252

