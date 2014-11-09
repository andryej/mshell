#define _POSIX_SOURCE 1

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

#include "config.h"
#include "cparse.h"

#define WSPACES " \t\n"
#define REDIR_CHARS "><"

int skip_red(char*);
int parse_redirection(char*, command_s*);
void trim_trailing_ws(char*);
void reset_command(command_s*);
int split_one_command(char*, char**, command_s*);

int skip_red(str) char* str;
{
  int base = 1;

  if (str[0] == 0) return 0;
  if (str[0] == '>' && str[1] == '>') base = 2;

  base += strspn(str + base, WSPACES);
  return base;
}

int parse_redirection(str, command) char* str;
command_s* command;
{
  char** field;

  if (!str[0]) return 0;

  if (str[0] == '<') field = &(command->in_file_name);
  if (str[0] == '>') {
    field = &(command->out_file_name);
    if (str[1] == '>') command->append_mode = 1;
  }

  if (*field) return 1; /* field already set */

  (*field) = str + skip_red(str);
  return 0;
}

void trim_trailing_ws(str) char* str;
{
  char* it, *last;

  if (!str) return;

  it = str;
  last = NULL;

  while (*it) {
    if (!isspace(*it)) last = it;
    it++;
  }
  if (last) (*(last + 1)) = 0;
}

void reset_command(command) command_s* command;
{
  command->in_file_name = NULL;
  command->out_file_name = NULL;
  command->argv = NULL;
  command->append_mode = 0;
}

int split_one_command(str, ibuff, command) char* str;
char** ibuff;
command_s* command;
{
  int i = 0, skip, fail = 0;

  int redir[2];

  reset_command(command);

  redir[0] = strcspn(str, REDIR_CHARS);
  skip = skip_red(str + redir[0]);

  redir[1] = redir[0] + skip + strcspn(str + redir[0] + skip, REDIR_CHARS);

  fail += parse_redirection(str + redir[1], command);
  str[redir[1]] = 0;

  fail += parse_redirection(str + redir[0], command);
  str[redir[0]] = 0;

  if (fail) {
    fprintf(stderr, "Parse error.\n");
    return -1;
  }

  trim_trailing_ws(command->in_file_name);
  trim_trailing_ws(command->out_file_name);

  command->argv = ibuff;

  ibuff[0] = strtok(str, WSPACES);

  if (!ibuff[0]) {
    return -1;
  }

  while (ibuff[i] && i < MAX_ARGS) {
    i++;
    ibuff[i] = strtok(NULL, WSPACES);
  }

  ibuff[i + 1] = NULL; /* in case of MAX_ARGS */
  return i;
}

command_s* split_commands(str) char* str;
{
  /* 0 ended array to store the sequence of commnands (argvs) */
  static command_s commands[MAX_COMMANDS + 1];

  char* command_strings[MAX_COMMANDS + 1];

  /* buffer to store 0 ended argvs */
  static char* buff_argv[MAX_COMMANDS * (MAX_ARGS + 1)];

  int i = 0, argc;
  char* command, **ibuff;

  command_strings[i] = strtok(str, "|");

  while (command_strings[i++] && i < MAX_COMMANDS) {
    command_strings[i] = strtok(NULL, "|");
  }
  command_strings[i] = NULL;

  ibuff = buff_argv;
  for (i = 0; command_strings[i]; i++) {
    argc = split_one_command(command_strings[i], ibuff, &commands[i]);
    if (argc < 0) return NULL;
    ibuff += argc + 1;
  }

  commands[i].argv = NULL;

  return commands;
}

int in_background(str) char* str;
{
  char* last = str;
  char* ind;

  if (!*str) return 0;

  for (ind = str; *ind != 0; ind++)
    if (!isspace(*ind)) last = ind;

  if (*last == '&') {
    *last = 0;
    return 1;
  };

  return 0;
}

#define STR_OR_NULL(str) str ? str : "NULL"

void print_command(command) command_s* command;
{
  int i;
  printf("Command: %s\n", STR_OR_NULL(command->argv[0]));

  printf("argv:");
  for (i = 0; command->argv[i]; i++) printf(" %s", command->argv[i]);

  printf("\n");
  printf("In file name:(%s)\n", STR_OR_NULL(command->in_file_name));
  printf("Out file name:(%s)\n", STR_OR_NULL(command->out_file_name));
  printf("Append mode:%d\n", command->append_mode);
}
