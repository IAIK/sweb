
#include "../common/include/kernel/syscall-definitions.h"

long syscall(long syscall_number, long arg0, long arg1, long arg2)
{
  long return_value;
  	__asm__ __volatile__ (
	   "int $0x80\n"
	   :"=a" (return_value)
	   :"a" (syscall_number), "b" (arg0), "c" (arg1), "d" (arg2)
  );
  return return_value;
}

void print_to_stdout(char *string)
{
  int strlen = 0;
  while (*string++)
    strlen++;
  
  syscall(sc_write,fd_stdout,(long) string,strlen);
}


void _start() 
{
  // push arguments to stack for function main
  
  // call main
  main();
  
  // call Exit syscall with arguments
  syscall(sc_exit,0,0,0);
}
