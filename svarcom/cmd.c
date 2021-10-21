/* entry point for internal commands
 * matches internal commands and executes them
 * returns -1 or exit code if processed
 * returns -2 if command unrecognized */

#include <i86.h>
#include <stdio.h>

#include "doserr.h"
#include "helpers.h"

#include "cmd/cd.c"
#include "cmd/set.c"

#include "cmd.h"

int cmd_process(int argc, const char **argv, unsigned short env_seg, const char far *cmdline) {

  if ((imatch(argv[0], "cd")) || (imatch(argv[0], "chdir"))) return(cmd_cd(argc, argv));
  if (imatch(argv[0], "set")) return(cmd_set(argc, argv, env_seg, cmdline));

  return(-2); /* command is not recognized as internal */
}
