/*
 * exit
 *
 * Quits the COMMAND.COM program (command interpreter)
 *
 */

static int cmd_exit(const struct cmd_funcparam *p) {
  if ((p->argc == 1) && (imatch(p->argv[0], "/?"))) {
    puts("EXIT");
    puts("");
    puts("Quits the COMMAND.COM program (command interpreter)");
    puts("");
  } else {
    exit(0);
  }
  return(-1);
}
