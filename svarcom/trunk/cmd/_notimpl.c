/*
 * handler for all "not implemented yet" commands
 */

static int cmd_notimpl(struct cmd_funcparam *p) {
  outputnl("This command is not implemented yet. Sorry!");
  return(-1);
}
