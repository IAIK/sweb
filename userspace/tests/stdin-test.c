#include "unistd.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "nonstd.h"

#define FORK_ENABLED 0
#define EXECUTABLE_PREFIX       "/"          // "/user_progs/"
#define EXECUTABLE_PREFIX_LEN   1            //12

int running;
int exit_code;

char command[256];
char executable[256+EXECUTABLE_PREFIX_LEN];
char args[10][256];

void handle_command(char* buffer,int buffer_size)
{
  int c=0;
  int num=0;
  int argsCount = -1;
  int lastIndex = 0;
  int pid;

  for(c=0; c<buffer_size; c++)
  {
    if(argsCount > 10)
    {
      argsCount = 10;
      printf("Argument Count is limited to 10 (no dynamic memory allocation) all other arguments will be ignores\n");
      break;
    }
    if(buffer[c] == '\r' || buffer[c] == '\n' || buffer[c] == ' ')
    {
      if(argsCount == -1)
      {
        memcpy(command, buffer + lastIndex, c - lastIndex);
        command[c - lastIndex] = 0;
      }
      else
      {
        memcpy(args[argsCount], buffer + lastIndex, c - lastIndex);
        args[argsCount][c - lastIndex] = 0;
      }
      argsCount++;
      lastIndex = c + 1;
      
      
    }
  }


  /*
  printf("Command: '%s' len: %d\n", command, strlen(command));

  for(c=0; c<argsCount; c++)
  {
    printf("Argument #%d: '%s' len: %d\n", c, args[c], strlen(args[c]));
  }
  */


  if (strcmp(command, "ls") == 0)
    printf("Sorry, Filesystem Syscalls not implemented yet\n");
  else if (strcmp(command, "ps") == 0)
    printf("Sorry, Threading System Syscalls not Implemented,\nuse F12 to print a list of Threads to Bochs\n");
  else if (buffer[0]=='h' && buffer[1] == 'e' && buffer[2] == 'l' && buffer[3] == 'p')
  {
    printf("Command Help:\nhelp                  yes, here we are\nexit [exit_code]      is really the only command that does something right now\nls                    does nothing\nps                    does nothing as well\ntest <num> [&& cmd]   evaluates integer x==0:false, x<>0:true, default: false\n\n");
  }
  else if (strcmp(command, "test") == 0)
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
  else if (strcmp(command, "exit") == 0)
  {
    c=4;
    while (buffer[c] ==' ')
      c++;
    exit_code=atoi(&buffer[c]);
    printf("Exiting Shell with exit_code %d\n",exit_code); 
    running=0;
  }
  else if (FORK_ENABLED)
  {
    pid = fork();
    printf("NewPid: %d\n", pid);

    if(pid == 0)
    {
      //child process, replace with new image
      execv(command,0);
      printf("Command not understood\n");
    }
    else
    {
      //wait for child process
      printf("Join on child process\n");
    }

    
  }
  else
  {
    memcpy(executable, EXECUTABLE_PREFIX, EXECUTABLE_PREFIX_LEN);
    memcpy(executable + EXECUTABLE_PREFIX_LEN, command, strlen(command) + 1);
    pid = createprocess(executable, 1);
    if (pid == -1)
    {
      printf("Command not understood\n");
    }
  }
}

int main(int argc, char *argv[]) 
{
  printf("Starting stdin test\n");
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
