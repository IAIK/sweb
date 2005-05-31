
char const * const some_ro_data = "some ro data";
char * data = "some data";
int _start(int argc, char *argv[])
{
	for(;;);
	return (int) some_ro_data + (int) data;
}
