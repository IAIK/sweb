int main() 
{
	int b = 3*2+1;
	int c;
	for (c=0; c< 16; ++c)
		b = b << 1;
	return b;
}
