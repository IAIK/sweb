
int foobar;

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
   }
   	return 0;
}
