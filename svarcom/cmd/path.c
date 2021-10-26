/*
 * path
 *
 * Displays or sets a search path for executable files.
 */

static int cmd_path(struct cmd_funcparam *p) {
  char *buff = p->BUFFER;

  /* no parameter - display current path */
  if (p->argc == 0) {
    char far *curpath = env_lookup(p->env_seg, "PATH");
    if (curpath == NULL) {
      outputnl("No Path");
    } else {
      unsigned short i;
      for (i = 0;; i++) {
        buff[i] = curpath[i];
        if (buff[i] == 0) break;
      }
      outputnl(buff);
    }
    return(-1);
  }

  /* more than 1 parameter */
  if (p->argc > 1) {
    outputnl("Too many parameters");
    return(-1);
  }

  /* IF HERE: THERE IS EXACTLY 1 ARGUMENT (argc == 1) */

  /* help screen (/?) */
  if (imatch(p->argv[0], "/?")) {
    output("Displays or sets a search path for executable files.\r\n"
           "\r\n"
           "PATH [[drive:]path[;...]]\r\n"
           "PATH ;\r\n"
           "\r\n"
           "Type PATH ; to clear all search-path settings and direct DOS to search\r\n"
           "only in the current directory.\r\n"
           "\r\n"
           "Type PATH without parameters to display the current path.\r\n");
    return(-1);
  }

  /* reset the PATH string (PATH ;) */
  if (imatch(p->argv[0], ";")) {
    env_dropvar(p->env_seg, "PATH");
    return(-1);
  }

  /* otherwise set PATH to whatever is passed on command-line */
  {
    unsigned short i;
    strcpy(buff, "PATH=");
    for (i = 0;; i++) {
      buff[i + 5] = p->argv[0][i];
      if (buff[i + 5] == '\r') break;
    }
    buff[i + 5] = 0;
    env_setvar(p->env_seg, buff);
  }

  return(-1);
}
