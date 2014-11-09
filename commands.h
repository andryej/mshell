typedef struct {
  char* name;
  int (*fun)(char**);
} command_pair;

extern command_pair dispatch_table[];
