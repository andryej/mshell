#define _POSIX_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#include "cparse.h"
#include "config.h"
#include "util.h"
#include "commands.h"

#define TRUE 1
#define FALSE 0

#define ERROR -1
#define EMPTY -1
#define NEW_LINE '\n'
#define EOL '\0'

#define IN 0
#define OUT 1

int it, jt, kt;
int status;
int result;
int printout_flag;
int backgrd_chld;
int forgrd_chld;
int left_in_fg;
int beg_it, end_it;
char buff[DBUFF_SIZE];
struct sigaction def_int_sga;
struct sigaction def_chld_sga;
struct sigaction nw_int_sga;
struct sigaction nw_chld_sga;
int fg_chld_pid[MAX_PROC_NUM];
chld_info chld_proc_tab[MAX_PROC_NUM];
sigset_t wt_sgset;
sigset_t blk_sgset;

void initialise(void) {
  struct stat fst;

  fstat(0, &fst);
  if (S_ISCHR(fst.st_mode)) {
    printout_flag = TRUE;
  }

  sigprocmask(SIG_BLOCK, NULL, &wt_sgset);
  sigemptyset(&blk_sgset);
  sigaddset(&blk_sgset, SIGCHLD);

  nw_int_sga.sa_handler = SIG_IGN;
  nw_chld_sga.sa_handler = nw_sigchld_hndl;

  sigfillset(&nw_int_sga.sa_mask);
  sigfillset(&nw_chld_sga.sa_mask);

  sigaction(SIGINT, &nw_int_sga, &def_int_sga);
  sigaction(SIGCHLD, &nw_chld_sga, &def_chld_sga);

  beg_it = end_it = it = 0;
  while (it < MAX_PROC_NUM) {
    chld_proc_tab[it++].pid = EMPTY;
  }
  return;
}

int raw_input(void) {
  int i;
  if (beg_it > BUFF_SIZE) {
    for (i = 0; beg_it + i < end_it; ++i) buff[i] = buff[beg_it + i];
    end_it -= beg_it;
    beg_it = 0;
  }
  return read(STDIN_FILENO, buff + end_it, BUFF_SIZE);
}

int work(void) {
  int len, status, ptout;
  ptout = 0;
  while (TRUE) {
    if (!ptout) {
      if ((status = display_prompt()) == ERROR) {
        continue;
      }
      ptout = TRUE;
    }
    len = raw_input();
    if (len == EOL) {
      if (printout_flag) my_write("exit\n");
      return 0;
    } else if (len == ERROR) {
      if (errno == EINTR) {
        continue;
      }
      return ERROR;
    } else {
      ptout = FALSE;
      end_it += len;
      parse_line();
    }
  }
  return 0;
}

void alter_fd(int wt, int rf, int* pp) {
  if (rf != ERROR) {
    dup2(rf, IN);
    close(rf);
  }
  if (wt != ERROR) {
    dup2(wt, OUT);
    close(wt);
    close(pp[OUT]);
  }
  return;
}

void file_manage(command_s* c) {
  mode_t mod;
  if (c->in_file_name != NULL) {
    close(IN);
    open(c->in_file_name, O_RDONLY);
  }
  if (c->out_file_name != NULL) {
    mod = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    close(OUT);
    if (c->append_mode)
      open(c->out_file_name, O_RDWR | O_CREAT | O_APPEND, mod);
    else
      open(c->out_file_name, O_RDWR | O_CREAT | O_TRUNC, mod);
  }
  return;
}

int internal_func(char* str, command_s* cmd) {
  int i;
  int status;
  int fd;
  int av_out;
  mode_t mod;

  for (i = 0; dispatch_table[i].name; ++i) {
    if (strcmp(str, dispatch_table[i].name) == 0) {
      if ((strcmp(str, "lenv") == 0) && (cmd->out_file_name != NULL)) {
        if (cmd->append_mode) {
          fd = open(cmd->out_file_name, O_RDWR | O_CREAT | O_APPEND, mod);
        } else {
          fd = open(cmd->out_file_name, O_RDWR | O_CREAT | O_TRUNC, mod);
        }
        av_out = dup(OUT);
        close(OUT);
        dup(fd);
        close(fd);
      }

      dispatch_table[i].fun(cmd->argv);

      if ((strcmp(str, "lenv") == 0) && (cmd->out_file_name != NULL)) {
        close(OUT);
        dup(av_out);
        close(av_out);
      }
      return TRUE;
    }
  }
  return FALSE;
}

void exec_line(void) {
  command_s* commnd;
  int pid;
  int pfd[2];
  int wt, rf;
  int in_bg;

  in_bg = in_background(&buff[beg_it]);
  commnd = split_commands(&buff[beg_it]);
  wt = rf = -1;

  if (commnd[0].argv == NULL) {
    return;
  }

  if (internal_func(commnd[0].argv[0], &commnd[0])) {
    return;
  }

  sigprocmask(SIG_BLOCK, &blk_sgset, NULL);
  it = 0;
  while (commnd[it].argv) {
    if (commnd[it + 1].argv) {
      pipe(pfd);
      wt = pfd[OUT];
    }

    /* Child */
    if (!(pid = fork())) {
      if (in_bg) setsid();

      sigaction(SIGINT, &def_int_sga, NULL);
      sigaction(SIGCHLD, &def_chld_sga, NULL);
      sigprocmask(SIG_UNBLOCK, &blk_sgset, NULL);

      alter_fd(wt, rf, pfd);
      file_manage(&commnd[it]);

      if (execvp(commnd[it].argv[0], commnd[it].argv) == ERROR) {
        exit(ERROR);
      }
    } /* Parent */
    else if (pid > 0) {
      if (wt != ERROR) close(wt);
      if (rf != ERROR) close(rf);
      fg_chld_pid[it] = pid;
    } else /* Error */
    {
      exit(ERROR);
    }

    rf = pfd[IN];
    wt = -1;
    ++it;
  }
  if (!in_bg) {
    left_in_fg = forgrd_chld = it;
    while (left_in_fg) {
      sigsuspend(&wt_sgset);
    }
    left_in_fg = forgrd_chld = 0;
  }
  sigprocmask(SIG_UNBLOCK, &blk_sgset, NULL);
  return;
}

void parse_line(void) {
  int i = 0;
  while (beg_it + i < end_it) {
    if (buff[beg_it + i] == NEW_LINE) {
      buff[beg_it + i] = EOL;
      exec_line();

      beg_it += i + 1;
      i = 0;
      continue;
    }
    ++i;
  }
  return;
}

int my_write(char* str) {
  int i = 0;
  int len = strlen(str);
  int status;
  while ((status = write(STDOUT_FILENO, str + i, len)) != EOF && (len > 0)) {
    if (status == ERROR) {
      if (errno == EINTR) continue;
      return ERROR;
    }

    i += status;
    len -= status;
  }
  return 0;
}

int display_prompt(void) {
  int i;
  char str[128];
  sigprocmask(SIG_BLOCK, &blk_sgset, NULL);
  if (printout_flag) {
    for (i = 0; i < MAX_PROC_NUM; ++i) {
      if (chld_proc_tab[i].pid != EMPTY) {
        if (WIFSIGNALED(chld_proc_tab[i].status)) {
          sprintf(str, "Process %d killed by %d\n", chld_proc_tab[i].pid,
                  WTERMSIG(chld_proc_tab[i].status));
        } else {
          sprintf(str, "Process %d ended with status %d\n",
                  chld_proc_tab[i].pid, chld_proc_tab[i].status);
        }
        chld_proc_tab[i].pid = EMPTY;
        chld_proc_tab[i].status = EMPTY;
        my_write(str);
      }
    }
    my_write(PROMPT);
  }
  sigprocmask(SIG_UNBLOCK, &blk_sgset, NULL);
  return 0;
}

void nw_sigint_hndl(int num) {
  if (printout_flag) my_write("\n");
}

void nw_sigchld_hndl(int num) {
  int bg;
  int i;
  int status;
  int pid;
  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    bg = TRUE;
    for (i = 0; i < forgrd_chld; ++i) {
      if (fg_chld_pid[i] == pid) {
        bg = FALSE;
        left_in_fg -= 1;
        break;
      }
    }

    if (bg && printout_flag) {
      it = 0;
      while ((it < MAX_PROC_NUM) && (chld_proc_tab[it].pid != EMPTY)) {
        ++it;
      }
      chld_proc_tab[it].pid = pid;
      chld_proc_tab[it].status = status;
    }
  }
  return;
}
