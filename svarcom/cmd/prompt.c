/*
 * prompt
 *
 * Changes the DOS command prompt.
 *
 */

static int cmd_prompt(const struct cmd_funcparam *p) {
  if ((p->argc == 1) && (imatch(p->argv[0], "/?"))) {
    output("Changes the DOS command prompt.\r\n"
           "\r\n"
           "PROMPT [new command prompt specification]\r\n"
           "\r\n");
    return(-1);
  }

  /* no parameter - restore default prompt path */
  if (p->argc == 0) {
    env_dropvar(p->env_seg, "PROMPT");
    return(-1);
  }

  /* otherwise set PROMPT to whatever is passed on command-line */
  {
    char buff[256] = "PROMPT=";
    unsigned short i;
    for (i = 0;; i++) {
      buff[i + 7] = p->cmdline[p->argoffset + i];
      if (buff[i + 7] == '\r') break;
    }
    buff[i + 7] = 0;
    env_setvar(p->env_seg, buff);
  }

  return(-1);
}
