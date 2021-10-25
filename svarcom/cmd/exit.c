/*
 * exit
 *
 * Quits the COMMAND.COM program (command interpreter)
 *
 */

static int cmd_exit(const struct cmd_funcparam *p) {
  if ((p->argc == 1) && (imatch(p->argv[0], "/?"))) {
    output("EXIT\r\n"
           "\r\n"
           "Quits the COMMAND.COM program (command interpreter)\r\n"
           "\r\n");
  } else {
    exit(0);
  }
  return(-1);
}
