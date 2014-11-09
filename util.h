
#define MAX_PROC_NUM 200

/* structure for holding information about expired child process */
typedef struct {
  int pid;
  int status;
} chld_info;

extern chld_info proc_tab[];

/* initialise required data structures */
void initialise(void);

/* main program loop */
int work(void);

/* read data from file specified in 0 file descriptor */
int raw_input(void);

/* change file decriptors in order to correct pipe communication */
void alter_fd(int wt, int rf, int* pp);

/* print out the shell prompt and status of ending for background child
 * processes */
int display_prompt(void);

/* after reading data check whether a new line sign has occured and try to parse 
 * it and execute */
void parse_line(void);

/* when a whole line has been read function tries to parse it and execute */
void exec_line(void);

/* new handler for interupt signal */
void nw_sigint_hndl(int num);

/* new handler for child signal */
void nw_sigchld_hndl(int num);

/* own implementation of write */
int my_write(char* str);
