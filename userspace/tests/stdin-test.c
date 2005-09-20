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
	char read_buffer[20];
	char* str1="Hi, ich bin ein Userspace Programm\n";
	char* str2="und ich warte auf 4 Tastendruecke\n";
	long read=0,read_total=0,c=0;
	
	print_to_stdout(str1);
	print_to_stdout(str2);
	
	while (read_total < 4)
	{
		read=__syscall(sc_read,fd_stdin,(long) (read_buffer+read_total), 5 - read_total);
		read_total+=read;
	}
	read_buffer[read_total+1]=0;

	print_to_stdout("Juhuu, habe erfolgreich 4 Tastendruecke gesammelt:>");
	print_to_stdout(read_buffer);
	print_to_stdout("<\n");
	
}
