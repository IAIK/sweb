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

typedef struct {
    const char* command;
    size_t str_len;
    size_t num_matches;
} autocomplete_result_t;

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

    for (size_t dpos = 0; dpos < static_cast<size_t>(ndents);)
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
        "ls [path]             pseudo ls\n\n");
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
      exit(1);
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

autocomplete_result_t autocomplete(const char* user_input, const char* path)
{
  autocomplete_result_t res;
  memset(&res, 0, sizeof(res));

  ssize_t ndents = getdentsBuffer(path, dirent_buffer, sizeof(dirent_buffer));

  for (size_t dpos = 0; dpos < static_cast<size_t>(ndents);)
  {
      dirent* dent = (dirent*)(dirent_buffer + dpos);

      if (strstr(dent->d_name, user_input) == dent->d_name)
      {
          ++res.num_matches;
          if (res.num_matches > 1)
              printf("\n%s", dent->d_name);
          if (!res.command)
          {
              res.command = dent->d_name;
              res.str_len = strlen(res.command);
          }
          else
          {
              size_t i = 0;
              while (i < res.str_len && res.command[i] && dent->d_name[i] && res.command[i] == dent->d_name[i])
                  ++i;
              res.str_len = i;
          }
      }

      dpos += dent->d_offs_next;
      if (dent->d_offs_next == 0)
          break;
  }

  for (size_t i = 0; i < sizeof(builtin_commands)/sizeof(builtin_commands[0]); ++i)
  {
      if (strstr(builtin_commands[i], user_input) == builtin_commands[i])
      {
          ++res.num_matches;
          if (res.num_matches > 1)
              printf("\n%s", builtin_commands[i]);
          if (!res.command)
          {
              res.command = builtin_commands[i];
              res.str_len = strlen(res.command);
          }
          else
          {
              size_t j = 0;
              while (j < res.str_len && res.command[j] && builtin_commands[i][j] && res.command[j] == builtin_commands[i][j])
                  ++j;
              res.str_len = j;
          }
      }
  }

  if (res.num_matches > 1)
      printf("\n%s", res.command);

  return res;
}

int readCommand(char* buffer, size_t buffer_size)
{
  unsigned int chars_in_buffer = 0;
  char cchar;
  char up_pressed = 0;

  memset(buffer, 0, buffer_size);

  while((cchar = getchar()) != EOF)
  {
    if(cchar == '\r' || cchar == '\n' || (chars_in_buffer + 1) >= buffer_size)
    {
      buffer[chars_in_buffer] = '\0';
      break;
    }
    else if(cchar == '\t') // autocomplete
    {
      if(chars_in_buffer == 0)
        continue;

      autocomplete_result_t res = autocomplete(buffer, EXECUTABLE_PREFIX);
      if(!res.command || !res.str_len)
        continue; // no such file

      if (res.num_matches > 1)
          printf("\nSWEB: %s> %s", cwd, buffer);

      const char* completion = res.command;
      completion += chars_in_buffer;

      int i;
      for(i = 0; completion[i] && completion[i] != '\n' && chars_in_buffer < res.str_len && chars_in_buffer < BUFFER_SIZE - 1; i++, chars_in_buffer++)
      {
          buffer[chars_in_buffer] = completion[i];
      }

      write(STDOUT_FILENO, completion, i);
      continue;
    }
    else if((unsigned char)cchar == 151) // up: take last input
    {
      if(chars_in_buffer != 0)
        continue;

      up_pressed = 1;

      chars_in_buffer = strlen(last_input);
      memcpy(buffer, last_input, buffer_size);
      write(STDOUT_FILENO, buffer, chars_in_buffer);
      continue;
    }
    else if((unsigned char)cchar == 152) // down: drop input
    {
      if(chars_in_buffer == 0 || !up_pressed)
        continue;

      memset(buffer, '\b', chars_in_buffer);
      buffer[chars_in_buffer] = 0;
      printf("%s",buffer);
      memset(buffer, 0, buffer_size);
      chars_in_buffer = 0;
      up_pressed = 0;
      continue;
    }
    else
    {
      buffer[chars_in_buffer] = (char)cchar;
    }

    if (cchar == '\b')
    {
      if (chars_in_buffer > 0)
      {
        buffer[chars_in_buffer] = 0;
        chars_in_buffer--;
      }
    }
    else
    {
      chars_in_buffer++;
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
    printf("\nSWEB: %s> ", cwd);
    readCommand(buffer, BUFFER_SIZE);
    handle_command(buffer, BUFFER_SIZE);
    for (size_t a = 0; a < BUFFER_SIZE; a++)
      buffer[a] = 0;

  } while (running);

  return exit_code;
}
