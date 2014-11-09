#define _POSIX_SOURCE 1

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <dirent.h>

#include "commands.h"
#include "util.h"

int echo(char * []);
int m_exit(char * []);
int m_cd(char * []);
int m_kill(char * []);
int m_lenv(char * []);
int m_ls(char * []);

command_pair dispatch_table[] = {{"echo", &echo},
                                 {"exit", &m_exit},
                                 {"cd", &m_cd},
                                 {"kill", &m_kill},
                                 {"lenv", &m_lenv},
                                 {"lls", &m_ls},
                                 {NULL, NULL}};

int is_int(char *str) {
  int i = 0;
  int len = strlen(str);
  if (str[0] == '-') i = 1;
  for (; i < len; ++i)
    if (str[i] < '0' || str[i] > '9') return 0;
  return 1;
}
int parse_int(char *a) {
  int i;
  int length;
  int beg;
  int result;

  beg = 0;
  beg += (a[0] == '-');
  result = 0;
  length = strlen(a);

  for (i = beg; i < length; i++) {
    result *= 10;
    result += a[i] - '0';
  }
  return beg ? -result : result;
}

int echo(char *argv[]) {
  int i;
  i = 1;

  if (argv[i]) my_write(argv[i++]);
  while (argv[i]) {
    my_write(" ");
    my_write(argv[i++]);
  }
  my_write("\n");
  fflush(stdout);
  return 0;
}

int m_exit(char *argv[]) {
  int i;
  int id;

  id = 0;
  if (argv[1]) {
    if (!is_int(argv[1])) {
      my_write("mshell: exit: ");
      my_write(argv[0]);
      my_write(": numeric argument required\n");
      exit(255);
    }

    id = parse_int(argv[1]);
    if (id < 0) {
      my_write("mshell: exit: Illegal number: ");
      my_write(argv[1]);
      my_write("\n");
      return -1;
    }
    exit(id);
  } else {
    exit(EXIT_SUCCESS);
  }
  return -1;
}

int m_cd(char *argv[]) {
  char path[1024];

  getcwd(path, 1024);

  if (argv[1]) {
    if (argv[1][0] == '~') {
      chdir(getenv("HOME"));
      if (strlen(argv[1]) > 1) {
        if (argv[1][1] != '/' ||
            (strlen(argv[1]) > 2 && chdir(argv[1] + 2) != 0)) {
          my_write("mshell: cd: ");
          my_write(argv[1]);
          my_write(": No such file or directory\n");
          chdir(path);
        }
      }
    } else if (chdir(argv[1]) != 0) {
      my_write("mshell: cd: ");
      my_write(argv[1]);
      my_write(": No such file or directory\n");
    }
  } else if (chdir(getenv("HOME")) != 0) {
    my_write("mshell: cd: ");
    my_write(argv[1]);
    my_write(": No such file or directory\n");
  }
  return 0;
}

int m_kill(char *argv[]) {
  if (!argv[1]) {
    my_write("kill: usage: kill [-signal] pid\n");
    return -1;
  }
  if (!argv[2]) {
    kill(parse_int(argv[1]), 15);
  } else {
    kill(parse_int(argv[2]), -1 * parse_int(argv[1]));
  }
  fflush(stdout);

  return 0;
}

extern char **environ;

int m_lenv(char *argv[]) {
  char **current;

  for (current = environ; *current; current++) {
    my_write(*current);
    my_write("\n");
  }
  fflush(stdout);
}

int m_ls(char *argv[]) {
  char dir_name[1024];
  struct dirent *file;
  DIR *cur_dir;

  getcwd(dir_name, 1024);
  cur_dir = opendir(dir_name);

  if (cur_dir = opendir(dir_name)) {
    while (file = readdir(cur_dir)) {
      if (file->d_name[0] != '.') {
        my_write(file->d_name);
        my_write("\n");
      }
    }
    closedir(cur_dir);
  }
  fflush(stdout);
}
