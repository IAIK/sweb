#include "unistd.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "nonstd.h"
#include "dirent.h"
#include "fcntl.h"
#include "assert.h"
#include "sys/syscall.h"

#define FORK_ENABLED 0
#define EXECUTABLE_PREFIX       "/usr/"
#define EXECUTABLE_PREFIX_LEN   5
#define BUFFER_SIZE 256

int running = 1;
int exit_code = 0;

char cwd[256];
char command[256];
char executable[256 + EXECUTABLE_PREFIX_LEN];
char args[10][256];
char buffer[BUFFER_SIZE] __attribute__((aligned(4096)));
char last_input[BUFFER_SIZE];

char dirent_buffer[4096];
ssize_t ndents = 0;

const char* builtin_commands[] = {"ls", "exit", "help"};

const char* d_type_str[] = { // see constants in dirent.h
    "F", // File
    "D", // Directory
    "L", // Link
    "C", // Chardev
    "B"  // Blockdev
};

ssize_t getdentsBuffer(const char* path, char* buffer, size_t buffer_size)
{
    if (!path)
        return -1;

    int usr_fd = open(path, O_RDONLY);
    if (usr_fd == -1)
    {
        return -1;
    }

    ssize_t ndents = getdents(usr_fd, dirent_buffer, sizeof(dirent_buffer));

    close(usr_fd);

    return ndents;
}

int ls(const char* path)
{
    ssize_t ndents = getdentsBuffer(path, dirent_buffer, sizeof(dirent_buffer));

    if (ndents < 0)
    {
        printf("Unable to get directory content for %s", path);
        return -1;
    }

    for (size_t dpos = 0; dpos < ndents;)
    {
        dirent* dent = (dirent*)(dirent_buffer + dpos);
        printf("[%s] %s\n", dent->d_type <= 4 ? d_type_str[dent->d_type] :
                                                "?",
                            dent->d_name);

        dpos += dent->d_offs_next;
        if (dent->d_offs_next == 0)
            break;
    }

    return 0;
}

void handle_command(char* buffer, int buffer_size)
{
  int c = 0;
  int argsCount = -1;
  int lastIndex = 0;
  int pid;

  for (c = 0; c < buffer_size; c++) //Do check for null termination at end of for loop to have one last run with \0
  {
    if (argsCount >= 10)
    {
      argsCount = 10;
      printf("Argument count is limited to 10 (no dynamic memory allocation) - all other arguments will be ignored\n");
      break;
    }
    if (buffer[c] == '\r' || buffer[c] == '\n' || buffer[c] == ' ' || buffer[c] == '\0')
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

    if (buffer[c] == '\0')
    {
      break;
    }
  }


  if (strcmp(command, "ls") == 0)
  {
    ls((argsCount > 0 ? args[0] : "."));
  }
  else if (strcmp(command, "help") == 0)
  {
    printf(
        "Command Help:\n"
        "help                  yes, here we are\n"
        "exit [exit_code]      is really the only command that does something right now\n"
        "ls                    pseudo ls\n\n");
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
    {
      memcpy(executable, command, strlen(command) + 1);
    }

    pid = createprocess(executable, 1);
    if (pid == -1)
    {
      printf("Command not understood\n");
    }
  }
}


const char* autocomplete(char* user_input)
{
  const char* autocompletion = NULL;

  ssize_t ndents = getdentsBuffer(EXECUTABLE_PREFIX, dirent_buffer, sizeof(dirent_buffer));

  for (size_t dpos = 0; dpos < ndents;)
  {
      dirent* dent = (dirent*)(dirent_buffer + dpos);

      if (strstr(dent->d_name, user_input) == dent->d_name)
      {
          if (autocompletion)
              return NULL; // ambiguous
          autocompletion = dent->d_name;
      }

      dpos += dent->d_offs_next;
      if (dent->d_offs_next == 0)
          break;
  }

  for (size_t i = 0; i < sizeof(builtin_commands)/sizeof(builtin_commands[0]); ++i)
  {
      if (strstr(builtin_commands[i], user_input) == builtin_commands[i])
      {
          if (autocompletion)
              return NULL; // ambiguous
          autocompletion = builtin_commands[i];
      }
  }

  return autocompletion;
}

int readCommand(char* buffer, int buffer_size)
{
  unsigned int counter = 0;
  char cchar;
  char up_pressed = 0;

  memset(buffer, 0, buffer_size);

  while((cchar = getchar()) != EOF)
  {
    if(cchar == '\r' || cchar == '\n' || (counter + 1) >= buffer_size)
    {
      buffer[counter] = '\0';
      break;
    }
    else if(cchar == '\t') // autocomplete
    {
      if(counter == 0)
        continue;

      const char* first_occurrence = autocomplete(buffer);
      if(first_occurrence == NULL)
        continue; // no such file

      first_occurrence += counter;

      int i;
      for(i = 0; first_occurrence[i] != '\n' && counter < BUFFER_SIZE - 1; i++, counter++)
        buffer[counter] = first_occurrence[i];

      write(STDOUT_FILENO, first_occurrence, i);
      continue;
    }
    else if((unsigned char)cchar == 151) // up: take last input
    {
      if(counter != 0)
        continue;

      up_pressed = 1;

      counter = strlen(last_input);
      memcpy(buffer, last_input, buffer_size);
      write(STDOUT_FILENO, buffer, counter);
      continue;
    }
    else if((unsigned char)cchar == 152) // down: drop input
    {
      if(counter == 0 || !up_pressed)
        continue;

      memset(buffer, '\b', counter);
      buffer[counter] = 0;
      printf("%s",buffer);
      memset(buffer, 0, buffer_size);
      counter = 0;
      up_pressed = 0;
      continue;
    }
    else
    {
      buffer[counter] = (char)cchar;
    }

    if (cchar == '\b')
    {
      if (counter > 0)
      {
        buffer[counter] = 0;
        counter--;
      }
    }
    else
    {
      counter++;
    }
  }

  buffer[BUFFER_SIZE - 1] = 0;
  memcpy(last_input, buffer, buffer_size);
  return 0;
}

int main(int argc, char *argv[])
{
  cwd[0] = '/';
  cwd[1] = '\0';

  printf("\n\%s\n", "SWEB-Pseudo-Shell starting...\n");

  do
  {
    printf("\n%s %s%s", "SWEB:", cwd, "> ");
    readCommand(buffer, BUFFER_SIZE);
    handle_command(buffer, BUFFER_SIZE);
    for (size_t a = 0; a < BUFFER_SIZE; a++)
      buffer[a] = 0;

  } while (running);

  return exit_code;
}
