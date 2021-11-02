/*
 * exit
 *
 * Quits the COMMAND.COM program (command interpreter)
 *
 */

static int cmd_exit(struct cmd_funcparam *p) {
  if (cmd_ishlp(p)) {
    outputnl("EXIT\r\n");
    outputnl("Quits the COMMAND.COM program (command interpreter)");
  } else {
    exit(0);
  }
  return(-1);
}
