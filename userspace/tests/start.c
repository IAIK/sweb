
#include "../../common/include/kernel/syscall-definitions.h"

void EXIT(int exit_code)
{
	__syscall(sc_exit,exit_code,0,0);
}

void _start() 
{
  main();
  EXIT(0);
}
