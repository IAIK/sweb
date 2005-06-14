
char const * const some_ro_data = "some ro data";
char * data = "some data";
char blubba[10000];
int _start(int argc, char *argv[])
{
   __asm__ __volatile__ (
   "int $0x80"
   :
   :);
	for(;;);
	return 0;
}
