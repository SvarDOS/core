/*
 * pause
 */


static int cmd_pause(struct cmd_funcparam *p) {
  if (cmd_ishlp(p)) {
    outputnl("Suspends processing of a batch program");
    outputnl("\r\nPAUSE");
    return(-1);
  }

  press_any_key();
  return(-1);
}
