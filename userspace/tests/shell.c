#include "unistd.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "nonstd.h"
#include "sys/syscall.h"

#define FORK_ENABLED 0
#define EXECUTABLE_PREFIX       "/usr/"
#define EXECUTABLE_PREFIX_LEN   5

int running = 1;
int exit_code = 0;

char cwd[256];
char command[256];
char executable[256 + EXECUTABLE_PREFIX_LEN];
char args[10][256];
char buffer[256] __attribute__((aligned(4096)));

void handle_command(char* buffer, int buffer_size)
{
  int c = 0;
  int argsCount = -1;
  int lastIndex = 0;
  int pid;

  for (c = 0; c < buffer_size && buffer[c]; c++)
  {
    if (argsCount > 10)
    {
      argsCount = 10;
      printf("Argument Count is limited to 10 (no dynamic memory allocation) all other arguments will be ignores\n");
      break;
    }
    if (buffer[c] == '\r' || buffer[c] == '\n' || buffer[c] == ' ')
    {
      if (argsCount == -1)
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
  if (strcmp(command, "ls") == 0)
    __syscall(sc_pseudols, (size_t) (argsCount > 0 ? args[0] : ""), 0, 0, 0, 0);
  else if (buffer[0] == 'h' && buffer[1] == 'e' && buffer[2] == 'l' && buffer[3] == 'p')
  {
    printf(
        "Command Help:\nhelp                  yes, here we are\nexit [exit_code]      is really the only command that does something right now\nls                    pseudo ls\n\n");
  }
  else if (strcmp(command, "exit") == 0)
  {
    c = 4;
    while (buffer[c] == ' ')
      c++;
    exit_code = atoi(&buffer[c]);
    printf("Exiting Shell with exit_code %d\n", exit_code);
    running = 0;
  }
  else if (FORK_ENABLED)
  {
    pid = fork();
    printf("NewPid: %d\n", pid);

    if (pid == 0)
    {
      //child process, replace with new image
      execv(command, 0);
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
    if (command[0] != '/')
    {
      memcpy(executable, EXECUTABLE_PREFIX, EXECUTABLE_PREFIX_LEN);
      memcpy(executable + EXECUTABLE_PREFIX_LEN, command, strlen(command) + 1);
    }
    else
      memcpy(executable, command, strlen(command) + 1);
    pid = createprocess(executable, 1);
    if (pid == -1)
    {
      printf("Command not understood\n");
    }
  }
}

int main(int argc, char *argv[])
{
  cwd[0] = '/';

  printf("\n\%s\n", "SWEB-Pseudo-Shell starting...\n");
  do
  {
    printf("\n%s %s%s", "SWEB:", cwd, "> ");
    gets(buffer, 255);
    buffer[255] = 0;
    handle_command(buffer, 256);
    for (size_t a = 0; a < 256; a++)
      buffer[a] = 0;

  } while (running);

  return exit_code;
}
