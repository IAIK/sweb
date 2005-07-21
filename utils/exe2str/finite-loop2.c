
int main(int argc, char *argv[]) 
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
	return b;	
}
