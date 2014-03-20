
BITS 64

section .text

global __syscall
__syscall:

  ; ok, first we have to do some little cleanup as every function should do
  push rbp ; the base pointer goes onto the stack so we can restore it alter
  mov rbp, rsp;
  
  ; ok, we have to save ALL registers we are now going to overwrite for 
  ; our syscall
  

  push rbx
  push rsi
  push rdi
  push rcx
  push r11

  mov rax, [rbp + 16]
  mov rdi, [rbp + 24]
  mov rsi, [rbp + 32]
  mov rdx, [rbp + 40]
  mov rcx, [rbp + 48]
  mov r8, [rbp + 56]

  syscall
  
  pop r11
  pop rcx
  pop rdi
  pop rsi
  pop rbx
  mov rsp, rbp
  pop rbp
  ret
  
