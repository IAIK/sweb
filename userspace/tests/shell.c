#include "unistd.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "nonstd.h"
#include "dirent.h"
#include "fcntl.h"
#include "sys/syscall.h"
#include "wait.h"
#include "assert.h"


#define SHELL_ENABLE_FORK_EXEC 0
#define SHELL_ENABLE_DEBUG_ARGS 0

#define SHELL_DIVIDER "--------------------------------------------------------------------------------"
#define SHELL_CMD_LINE_PREFIX "SWEB: /> "
#define SHELL_EXECUTABLE_PREFIX "/usr/"
#define SHELL_KEY_UP (char)151
#define SHELL_KEY_DOWN (char)152

#define SHELL_MAX_ARGS 10
#define SHELL_BUFFER_SIZE 256
#define SHELL_DIR_CONTENT_SIZE 8192


int running = 1;
int exit_code = 0;

char buffer[SHELL_BUFFER_SIZE];
char command[SHELL_BUFFER_SIZE];
char last_command[SHELL_BUFFER_SIZE];
char args[SHELL_MAX_ARGS][SHELL_BUFFER_SIZE];
char dirent_buffer[SHELL_DIR_CONTENT_SIZE];
ssize_t ndents = 0;

typedef struct
{
    const char* command;
    size_t str_len;
    size_t num_matches;
} autocomplete_result_t;

const char* sweb = \
"  _____              _\n"
" /  ___|            | |\n"
" \\ `--.__      _____| |__\n"
"  `--. \\ \\ /\\ / / _ \\ '_ \\ \n"
" /\\__/ /\\      /  __/ |_) |\n"
" \\____/  \\_/\\_/ \\___|____/\n";

const char* builtin_commands[] = {"ls", "exit", "help", "clear"};

const char* d_type_str[] = {
    // see constants in dirent.h
    "F", // File
    "D", // Directory
    "L", // Link
    "C", // Chardev
    "B"  // Blockdev
};

/// --------------------------------
///         Helper Functions
/// --------------------------------

size_t copyAndFlushBuffer(char* src_buffer, char* dst_buffer) {
  memcpy(dst_buffer, src_buffer, SHELL_BUFFER_SIZE);
  memset(src_buffer, 0, SHELL_BUFFER_SIZE);
  size_t length_string = strlen(dst_buffer);
  assert(length_string < SHELL_BUFFER_SIZE);
  return length_string;
}

// FIXME: This function is broken. Cannot get the size of an array via a pointer
int isCharArrayFilledWithZeroes(const char* array) {
  int array_size = sizeof(array);
  for(int i = 0; i < array_size; ++i) {
    if(array[i]) {
      return 0;
    }
  }
  return 1;
}

void clearLine() {
  char backslashes[] = {[0 ... sizeof(SHELL_CMD_LINE_PREFIX) + SHELL_BUFFER_SIZE - 1] = '\b'};
  write(STDOUT_FILENO, backslashes, sizeof(backslashes));
}

void printDivider() {
  clearLine();
  printf("%s\n", SHELL_DIVIDER);
}


int splitCommandAndArguments() {
  assert(buffer[SHELL_BUFFER_SIZE - 1] == 0);
  char* first_space = strstr(buffer, " ");
  if(!first_space) {
    memcpy(command, buffer, SHELL_BUFFER_SIZE);
    command[SHELL_BUFFER_SIZE - 1] = 0;
    return 0;
  }

  int arg_counter = 0;
  memset(command, 0, SHELL_BUFFER_SIZE);
  memset(args, 0, SHELL_MAX_ARGS * SHELL_BUFFER_SIZE);
  memcpy(command, buffer, (size_t)first_space - (size_t)buffer);
  while(first_space && first_space + 1 < buffer + SHELL_BUFFER_SIZE && arg_counter < SHELL_MAX_ARGS) {
    char* next_space = strstr(first_space + 1, " ");
    size_t arg_length = !next_space ? strlen(first_space + 1) : (size_t)next_space - ((size_t)first_space + 1);
    assert(arg_length < SHELL_BUFFER_SIZE);
    if(arg_length) {
      memcpy(args[arg_counter++], first_space + 1, arg_length);
    }
    first_space = strstr(first_space + 1, " ");
  }

  command[SHELL_BUFFER_SIZE - 1] = 0;
  for(int i = 0; i < SHELL_MAX_ARGS; ++i) {
    args[i][SHELL_BUFFER_SIZE - 1] = 0;
  }
  return arg_counter;
}

ssize_t getdentsBuffer(const char* path, char* buffer, size_t buffer_size)
{
    if (!path)
        return -1;

    int usr_fd = open(path, O_RDONLY);
    if (usr_fd == -1)
    {
        printf("Dir open failed\n");
        return -1;
    }

    ssize_t ndents = getdents(usr_fd, buffer, buffer_size);

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

    for (size_t dpos = 0; dpos < (size_t)(ndents);)
    {
        dirent* dent = (dirent*)(dirent_buffer + dpos);
        printf("[%s] %s\n", dent->d_type <= 4 ? d_type_str[dent->d_type] : "?",
               dent->d_name);

        dpos += dent->d_offs_next;
        if (dent->d_offs_next == 0)
            break;
    }

    return 0;
}

autocomplete_result_t autocomplete(const char* user_input, const char* path)
{
    autocomplete_result_t res;
    memset(&res, 0, sizeof(res));

    ssize_t ndents = getdentsBuffer(path, dirent_buffer, sizeof(dirent_buffer));

    for (size_t dpos = 0; dpos < (size_t)(ndents);)
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
                while (i < res.str_len && res.command[i] && dent->d_name[i] &&
                       res.command[i] == dent->d_name[i])
                    ++i;
                res.str_len = i;
            }
        }

        dpos += dent->d_offs_next;
        if (dent->d_offs_next == 0)
            break;
    }

    for (size_t i = 0; i < sizeof(builtin_commands) / sizeof(builtin_commands[0]); ++i)
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
                while (j < res.str_len && res.command[j] && builtin_commands[i][j] &&
                       res.command[j] == builtin_commands[i][j])
                    ++j;
                res.str_len = j;
            }
        }
    }

    if (res.num_matches > 1)
        printf("\n%s", res.command);

    return res;
}

void printAvailableCommands() {
  printf("\n"
         " - help or h              | List all commands\n"
         " - ls or l <dir>          | List specified directory\n"
         " - <tc-name>.sweb         | Execute specified testcase\n");
  printf(" - Tab                    | Testcase autocompletion\n"
         " - ArrowUp, ArrowDown     | Copy last command, drop command\n"
         " - cls or clear           | Clear screen\n"
         " - exit or q <exit-code>  | Exit the shell with the specified exit-code\n\n");
}


/// ---------------------------------
///          Command Handling
/// ---------------------------------


void readCommand() {
  printf("\n%s", SHELL_CMD_LINE_PREFIX);
  size_t counter = 0;
  char temp_buffer[SHELL_BUFFER_SIZE] = {[SHELL_BUFFER_SIZE - 1] = 0};
  memset(buffer, 0, SHELL_BUFFER_SIZE);

  int keep_reading = 1;
  while(keep_reading && counter < SHELL_BUFFER_SIZE) {
    int last_command_buffer_empty = isCharArrayFilledWithZeroes(last_command);
    int temp_buffer_empty = isCharArrayFilledWithZeroes(temp_buffer);
    char input_char = (char)getchar();
    if (input_char == EOF) {
      keep_reading = 0;
      continue;
    }

    switch (input_char) {
      case '\r':
      case '\n':
        if(!temp_buffer_empty) {
          counter = copyAndFlushBuffer(temp_buffer, buffer) - 1;
          keep_reading = 0;
          break;
        }
        buffer[counter] = 0;
        keep_reading = 0;
        continue;

      case '\b':
        if(!temp_buffer_empty) {
          counter = copyAndFlushBuffer(temp_buffer, buffer);
        }
        buffer[counter] = 0;
        if(!counter) {
          // Prevent annoying "bug" where the prefix is overwritten by backslashes
          clearLine();
          printf("%s", SHELL_CMD_LINE_PREFIX);
          break;
        }
        counter--;
        break;
      case '\t':
      {
        if (counter == 0) {
            printDivider();
            ls(SHELL_EXECUTABLE_PREFIX);
            printf("\n\n%s", SHELL_CMD_LINE_PREFIX);
            break;
        }

        autocomplete_result_t res = autocomplete(buffer, SHELL_EXECUTABLE_PREFIX);
        if (!res.command || !res.str_len)
            continue; // no such file

        if (res.num_matches > 1)
            printf("\n%s%s", SHELL_CMD_LINE_PREFIX, buffer);

        const char* completion = res.command;
        completion += counter;

        int i;
        for (i = 0;
                completion[i] && completion[i] != '\n' &&
                counter < res.str_len && counter < SHELL_DIR_CONTENT_SIZE - 1;
                i++, counter++)
        {
            buffer[counter] = completion[i];
        }

        write(STDOUT_FILENO, completion, i);
        break;
      }
      case SHELL_KEY_UP:
        if(last_command_buffer_empty) {
          break;
        }
        clearLine();
        printf("%s", SHELL_CMD_LINE_PREFIX);
        memcpy(buffer, last_command, SHELL_BUFFER_SIZE);
        counter = strlen(buffer);
        write(STDOUT_FILENO, buffer, counter);
        memset(temp_buffer, 0, SHELL_BUFFER_SIZE);
        break;
      case SHELL_KEY_DOWN:
        clearLine();
        printf("%s", SHELL_CMD_LINE_PREFIX);
        memset(buffer, 0, SHELL_BUFFER_SIZE);
        memset(temp_buffer, 0, SHELL_BUFFER_SIZE);
        counter = 0;
        break;
      default:
        if(!temp_buffer_empty) {
          counter = copyAndFlushBuffer(temp_buffer, buffer);
        }
        buffer[counter++] = input_char;
        if(counter == SHELL_BUFFER_SIZE) {
          printf("\n");
        }
        break;
    }
  }
  buffer[SHELL_BUFFER_SIZE - 1] = 0;
  memcpy(last_command, buffer, SHELL_BUFFER_SIZE);
}

void handleCommand() {
  int arg_counter = splitCommandAndArguments();
  if(SHELL_ENABLE_DEBUG_ARGS) {
    printf("Captured Command = %s (%ld)\n", command, strlen(command));
    for(int i = 0; i < arg_counter; ++i) {
      // Split up due to 256 byte printf limit
      printf("Arg #%d: ", i  + 1);
      printf("%s", args[i]);
      printf(" (%ld)\n", strlen(args[i]));
    }
  }

  if (strcmp(command, "ls") == 0 || strcmp(command, "l") == 0) {
    ls((arg_counter > 0 ? args[0] : "."));
    return;
  }

  if(strcmp(command, "help") == 0 || strcmp(command, "h") == 0) {
    printAvailableCommands();
    return;
  }

  if(strcmp(command, "exit") == 0 || strcmp(command, "q") == 0 || strcmp(command, "quit") == 0) {
    exit_code = atoi(args[0]); // NOLINT(cert-err34-c)
    printf("Exiting Shell with exit_code %d\n", exit_code);
    running = 0;
    return;
  }

  if(strcmp(command, "cls") == 0 || strcmp(command, "clear") == 0) {
    // (Only) Enough for option 0 defined in menu.lst
    char cls[] = {[0 ... 20] = '\n'};
    printf("%s\n", cls);
    return;
  }

  char executable[sizeof(SHELL_EXECUTABLE_PREFIX) + SHELL_BUFFER_SIZE];
  memcpy(executable, SHELL_EXECUTABLE_PREFIX, strlen(SHELL_EXECUTABLE_PREFIX));
  memcpy(executable + strlen(SHELL_EXECUTABLE_PREFIX), command, strlen(command) + 1);

  if(!SHELL_ENABLE_FORK_EXEC) {
    if(createprocess(executable, 1) != 0) {
      // Split up due to 256 byte printf limit
      printf("Unknown command: ");
      printf("%s", command);
      printf(" (%ld)\n ", strlen(command));
    }
    return;
  }

  pid_t pid = fork();
  switch (pid) {
    case -1:
      assert(0 && "Fork returned -1, shell is unusable");
      break;
    case 0: {
      char* argv[arg_counter + 1];
      for(int i = 0; i < arg_counter; ++i) {
        argv[i] = args[i];
      }
      argv[arg_counter] = 0;
      execv(executable, argv);
      printf("Unknown command\n");
      exit(-1);
      break;
    }
    default:
      waitpid(pid, 0x0, 0x0);
      break;
  }
}

int main(int argc, char* argv[]) {
  printf("\n%s\nSWEB-Pseudo-Shell Starting...\n", sweb);
  printAvailableCommands();

  while(running) {
    readCommand();
    handleCommand();
  }
  return exit_code;
}
