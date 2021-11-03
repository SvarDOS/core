/*
 * rem
 */

static int cmd_rem(struct cmd_funcparam *p) {
  /* help screen ONLY if /? is the only argument - I do not want to output
   * help for ex. for "REM mouse.com /?" */
  if ((p->argc == 1) && (imatch(p->argv[0], "/?"))) {
    outputnl("Records comments (remarks) in a batch file or CONFIG.SYS");
    outputnl("");
    outputnl("REM [comment]");
  }
  return(-1);
}
