#include "unistd.h"
#include "stdio.h"

int running;
int exit_code;

void handle_command(char* buffer,int buffer_size)
{
	int c=0;
	int num=0;
	if (buffer[0]=='l' && buffer[1] =='s' && (buffer[2] == '\n' ||buffer[2] == '\r'|| buffer[2] == ' '))
		printf("Sorry, Filesystem Syscalls not implemented yet\n");
	else if (buffer[0] == 'p' && buffer[1] == 's' && (buffer[2] == '\n' ||buffer[2] == '\r'|| buffer[2] == ' '))
		printf("Sorry, Threading System Syscalls not Implemented,\nuse F12 to print a list of Threads to Bochs\n");
	else if (buffer[0]=='h' && buffer[1] == 'e' && buffer[2] == 'l' && buffer[3] == 'p')
	{
		printf("Command Help:\nhelp                  yes, here we are\nexit [exit_code]      is really the only command that does something right now\nls                    does nothing\nps                    does nothing as well\ntest <num> [&& cmd]   evaluates integer x==0:false, x<>0:true, default: false\n\n");
	}
	else if (buffer[0]=='t' && buffer[1] == 'e' && buffer[2] == 's' && buffer[3] == 't')
	{
		c=4;
		while (buffer[c] ==' ')
			c++;
		num=atoi(&buffer[c]);
		if (num==0)
			printf("Result: false\n"); 
		else
		{
			printf("Result: true\n"); 
			while (c < buffer_size)
				if (buffer[c] =='&' && buffer[c+1]=='&')
				{
					c+=2;
					while (buffer[c] ==' ')
						c++;
					handle_command(&buffer[c],buffer_size-c);
					break;
				}
				else
					c++;
		}
	}
	else if (buffer[0]=='e' && buffer[1] == 'x' && buffer[2] == 'i' && buffer[3] == 't')
	{
		c=4;
		while (buffer[c] ==' ')
			c++;
		exit_code=atoi(&buffer[c]);
		printf("Exiting Shell with exit_code %d\n",exit_code); 
		running=0;
	}
	else
		printf("Command not understood\n");
}

int main(int argc, char *argv[]) 
{
	char* str1="SWEB-Pseudo-Shell starting...\n";
	char* preprompt="SWEB:";
	char* prompt="> ";
	char buffer[256];
	int a=0;
	char cwd[256]="/";
	exit_code=0;
	running = 1;
	printf("\n\%s\n",str1);
	do 
	{
		printf("\n%s %s%s",preprompt,cwd,prompt);
		gets(buffer,255);
		buffer[255]=0;
		handle_command(buffer,256);
		for (a=0; a<256; a++)
			buffer[a]=0;
		
	} while (running);

	return exit_code;
}
