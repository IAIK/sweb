
void syscall(int syscall_number, int arg0, int arg1, int arg2)
{
  	__asm__ __volatile__ (
	   "int $0x80\n"
	   :
	   :"a" (syscall_number), "b" (arg0), "c" (arg1), "d" (arg2)
  );  
}

void _start() 
{
  // push arguments to stack for function main
  
  // call main
  main();
  
  // call Exit syscall with arguments
  syscall(0x0000dead,0,0,0);
}
