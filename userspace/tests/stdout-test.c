#include "unistd.h"
#include "stdio.h"

int main(int argc, char *argv[]) 
{
	char* str1="Hi, ich bin ein Userspace Programm\n";
	char* str2="und ich schreibe einen String auf die Console\n";
	char* str3="bye bye\n";

	
	printf("%s",str1);
	printf("%s",str2);
	printf("%s",str3);
}
