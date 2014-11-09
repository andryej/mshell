typedef struct command {
  char** argv;
  char* in_file_name;
  char* out_file_name;
  int append_mode;
} command_s;

/*
 * Checks whether the last non white space symbol in argument string is '&'
 * and replaces it by 0.
 * Argument is supposed to be 0 ended string.
 * Function should be called before split_commands.
 */
int in_background(char*);

/*
 * Parses sequence of commands in the given 0 ended string.
 * Commands are separated by '|'.
 * Returns pointer to the static array of structures command_s or NULL if meets
 * a parse error.
 * The first empty structure in the array has argv == NULL.
 * The input string is modified by the function and its substrings are used to
 * fill command_s structures.
 */
command_s* split_commands(char*);

void print_command(command_s*);
