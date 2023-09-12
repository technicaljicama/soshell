#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>
#include <dirent.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

/*
  Function Declarations for builtin shell commands:
 */
int soshell_cd(char **args);
int soshell_ls(char **args);
int soshell_rm(char **args);
int soshell_help(char **args);
int soshell_exit(char **args);

/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
  "cd",
  "ls",
  "rm",
  "help",
  "exit"
};

int (*builtin_func[]) (char **) = {
  &soshell_cd,
  &soshell_ls,
  &soshell_rm,
  &soshell_help,
  &soshell_exit
};

int soshell_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/

/**
   @brief Builtin command: change directory.
   @param args List of args.  args[0] is "cd".  args[1] is the directory.
   @return Always returns 1, to continue executing.
 */
int soshell_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "soshell: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("soshell");
    }
  }
  return 1;
}

int get_next(DIR* stream) {
  struct dirent* entry = readdir(stream);
  if(entry == NULL) {
    closedir(stream);
    return 0;
  }

  printf("%s\n", entry->d_name);
  return 1;
}

/**
   @brief Builtin command: list directory
   @param args List of args. args[0] is "ls". args[1] is the directory
   @return Always returns 1
**/
int soshell_ls(char **args)
{
  DIR* stream;

  if(args[1] == NULL) {
    stream = opendir(".");
  } else {
    stream = opendir(args[1]);
    if(stream == NULL) {
      printf("Unknown directory %s\n", args[1]);
      return 1;
    }
  }

  while(get_next(stream)) {}

  return 1;
}

/**
   @brief Builtin command remove file
   @param args list. args[1] will be the file ti remove
   @return Always returns 1
*/
int soshell_rm(char **args) {
  if(args[1] == NULL) {
    printf("You must provide a file\n");
    return 1;
  }

  int ret = remove(args[1]);
  if(ret != 0) {
    printf("Could not remove file.\n");
  }

  return 1;
}

/**
   @brief Builtin command: print help.
   @param args List of args.  Not examined.
   @return Always returns 1, to continue executing.
 */
int soshell_help(char **args)
{
  int i;
  printf("Soviet Linux soshell\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < soshell_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

/**
   @brief Builtin command: exit.
   @param args List of args.  Not examined.
   @return Always returns 0, to terminate execution.
 */
int soshell_exit(char **args)
{
  return 0;
}

/**
  @brief Launch a program and wait for it to terminate.
  @param args Null terminated list of arguments (including program).
  @return Always returns 1, to continue execution.
 */
int soshell_launch(char **args)
{
  pid_t pid;
  int status;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("soshell");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("soshell");
  } else {
    // Parent process
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

/**
   @brief Execute shell built-in or launch program.
   @param args Null terminated list of arguments.
   @return 1 if the shell should continue running, 0 if it should terminate
 */
int soshell_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < soshell_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return soshell_launch(args);
}

/**
   @brief Read a line of input from stdin.
   @return The line from stdin.
 */
char *soshell_read_line(void)
{
#ifdef SOSHELL_USE_STD_GETLINE
  char *line = NULL;
  ssize_t bufsize = 0; // have getline allocate a buffer for us
  if (getline(&line, &bufsize, stdin) == -1) {
    if (feof(stdin)) {
      exit(EXIT_SUCCESS);  // We received an EOF
    } else  {
      perror("soshell: getline\n");
      exit(EXIT_FAILURE);
    }
  }
  return line;
#else
#define LSH_RL_BUFSIZE 1024
  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "soshell: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // Read a character
    c = getchar();

    if (c == EOF) {
      exit(EXIT_SUCCESS);
    } else if (c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += LSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "soshell: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
#endif
}

#define SOSHELL_TOK_BUFSIZE 64
#define SOSHELL_TOK_DELIM " \t\r\n\a"
/**
   @brief Split a line into tokens (very naively).
   @param line The line.
   @return Null-terminated array of tokens.
 */
char **soshell_split_line(char *line)
{
  int bufsize = SOSHELL_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;

  if (!tokens) {
    fprintf(stderr, "soshell: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, SOSHELL_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += SOSHELL_TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
		free(tokens_backup);
        fprintf(stderr, "soshell: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, SOSHELL_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

/**
   @brief Loop getting input and executing it.
 */
void soshell_loop(void)
{
 struct utsname buffer;
 if (uname(&buffer) < 0){
   perror("uname");
   exit(EXIT_FAILURE);
  }
  char workdir[100];
  char *line;
  char **args;
  int status;

  do {
    printf(ANSI_COLOR_RED "%s" ANSI_COLOR_RESET,  buffer.nodename);
    printf(ANSI_COLOR_GREEN " [%s]$ " ANSI_COLOR_RESET, getcwd(workdir, 100));
    line = soshell_read_line();
    args = soshell_split_line(line);
    status = soshell_execute(args);

    free(line);
    free(args);
  } while (status);
}

/**
   @brief Main entry point.
   @param argc Argument count.
   @param argv Argument vector.
   @return status code
 */
int main(int argc, char **argv)
{
  // Load config files, if any.

  // Run command loop.
  soshell_loop();

  // Perform any shutdown/cleanup.

  return EXIT_SUCCESS;
}

