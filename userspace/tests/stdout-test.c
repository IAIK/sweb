#include "../../common/include/kernel/syscall-definitions.h"

void print_to_stdout(char *string)
{
  int strlen = 0;
  char *string2=string;
  while (*string2++)
    strlen++;
  
  __syscall(sc_write,fd_stdout,(long) string,strlen);
}

int main(int argc, char *argv[]) 
{
	char* str1="Hi, ich bin ein Userspace Programm\n";
	char* str2="und ich schreibe einen String auf die Console\n";
	char* str3="bye bye\n";

	
	print_to_stdout(str1);
	print_to_stdout(str2);
	print_to_stdout(str3);
}
