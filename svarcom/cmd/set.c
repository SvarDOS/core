/*
 * set [varname[=value]]
 *
 * value cannot contain any '=' character, but it can contain spaces
 * varname can also contain spaces
 */


static int cmd_set(struct cmd_funcparam *p) {
  char far *env = MK_FP(p->env_seg, 0);
  char *buff = p->BUFFER;
  /* no arguments - display content */
  if (p->argc == 0) {
    while (*env != 0) {
      unsigned short i;
      /* copy string to local buff for display */
      for (i = 0;; i++) {
        buff[i] = *env;
        env++;
        if (buff[i] == 0) break;
      }
      outputnl(buff);
    }
  } else if ((p->argc == 1) && (imatch(p->argv[0], "/?"))) {
    outputnl("Displays, sets, or removes DOS environment variables");
    outputnl("");
    outputnl("SET [variable=[string]]");
    outputnl("");
    outputnl("variable  Specifies the environment-variable name");
    outputnl("string    Specifies a series of characters to assign to the variable");
    outputnl("");
    outputnl("Type SET without parameters to display the current environment variables.");
  } else { /* set variable (do not rely on argv, SET has its own rules...) */
    const char far *ptr;
    unsigned short i;
    /* locate the first space */
    for (ptr = p->cmdline; *ptr != ' '; ptr++);
    /* now locate the first non-space: that's where the variable name begins */
    for (; *ptr == ' '; ptr++);
    /* copy variable to buff and switch it upercase */
    i = 0;
    for (; *ptr != '='; ptr++) {
      if (*ptr == '\r') goto syntax_err;
      buff[i] = *ptr;
      if ((buff[i] >= 'a') && (buff[i] <= 'z')) buff[i] -= ('a' - 'A');
      i++;
    }

    /* copy value now */
    while (*ptr != '\r') {
      buff[i++] = *ptr;
      ptr++;
    }

    /* terminate buff */
    buff[i] = 0;

    /* commit variable to environment */
    i = env_setvar(p->env_seg, buff);
    if (i == ENV_INVSYNT) goto syntax_err;
    if (i == ENV_NOTENOM) outputnl("Not enough available space within the environment block");
  }
  return(-1);

  syntax_err:

  outputnl("Syntax error");
  return(-1);
}
