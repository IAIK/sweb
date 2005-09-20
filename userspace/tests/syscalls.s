
BITS 32

section .text

global __syscall
__syscall:

  ; ok, first we have to do some little cleanup as every function should do
  push ebp 
  ; the base pointer goes onto the stack so we can restore it alter
  mov ebp, esp; 
  
  ; ok, we have to save ALL registers we are now going to overwrite for 
  ; our syscall
  

  push ebx;
  push esi;
  push edi;

  mov eax, [ebp + 8]
  mov ebx, [ebp + 12]
  mov ecx, [ebp + 16]
  mov edx, [ebp + 20]
  mov esi, [ebp + 24]
  mov edi, [ebp + 28]

  int 0x80;
  
  pop edi
  pop esi
  pop ebx
  mov esp, ebp
  pop ebp
  ret
  
