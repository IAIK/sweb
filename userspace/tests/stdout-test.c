#include "unistd.h"
#include "stdio.h"

//~ void print_to_stdout(char *string)
//~ {
  //~ int strlen = 0;
  //~ char *string2=string;
  //~ while (*string2++)
    //~ strlen++;
  
  //~ __syscall(sc_write,fd_stdout,(long) string,strlen);
//~ }

int main(int argc, char *argv[]) 
{
	char* str1="Hi, ich bin ein Userspace Programm\n";
	char* str2="und ich schreibe einen String auf die Console\n";
	char* str3="bye bye\n";

	
	printf("%s",str1);
	printf("%s",str2);
	printf("%s",str3);
}
