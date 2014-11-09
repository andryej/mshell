#define _POSIX_SOURCE 1

#include "config.h"
#include "commands.h"
#include "cparse.h"
#include "util.h"

int main(argc, argv) int argc;
char* argv[];
{
  initialise();
  return work();
}
