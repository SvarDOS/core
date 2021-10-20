/* entry point for internal commands
 * matches internal commands and executes them
 * returns -1 or exit code if processed
 * returns -2 if command unrecognized */

#include <i86.h>
#include <stdio.h>

#include "helpers.h"

#include "cmd/set.c"

#include "cmd.h"

int cmd_process(int argc, const char **argv, unsigned short env_seg) {

  if (imatch(argv[0], "set")) return(cmd_set(argc, argv, env_seg));

  return(-2); /* command is not recognized as internal */
}
