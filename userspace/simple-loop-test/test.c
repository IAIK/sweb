
int foobar;
typedef unsigned int uint32;

extern  __syscall(uint32 syscall_number, uint32 arg1, uint32 arg2, uint32 arg3, uint32 arg4, uint32 arg5);

int bartar(int blubba, int blabba)
{
	return (blubba)?bartar(--blubba,--blabba)+1:blubba++;
}

int _start(int argc, char *argv[])
{
   foobar = 0;
   for (;;)
   {
	foobar=bartar(foobar,20);
   	__syscall(0,1,2,3,4,5);
   }
   	return 0;
}
