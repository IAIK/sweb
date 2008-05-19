int _start() 
{
	int a=0;
	int b = 3*2+1;
	int c;
	
	for (a=0; a< 2000; ++a)
	{
		b=1;
		for (c=0; c< 16; ++c)
			b = b << 1;
	}
	
//call syscall handler, tell kernel that I'm dead
	__asm__ __volatile__ (
	   "movl $0xdeaddead, %%eax\n"
	   "push %%eax\n"
	   "int $0x80\n"
	   :
	   :);
	
}
