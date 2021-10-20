/*
 * set [varname[=value]]
 *
 * value cannot contain any '=' character, but it can contain spaces
 * varname can also contain spaces
 */

static int cmd_set(int argc, char const **argv, unsigned short env_seg, const char far *cmdline) {
  char far *env = MK_FP(env_seg, 0);
  char buff[256];
  int i;
  /* no arguments - display content */
  if (argc == 1) {
    while (*env != 0) {
      /* copy string to local buff for display */
      for (i = 0;; i++) {
        buff[i] = *env;
        env++;
        if (buff[i] == 0) break;
      }
      puts(buff);
    }
  } else if ((argc == 2) && (imatch(argv[1], "/?"))) {
    puts("TODO: help screen"); /* TODO */
  } else { /* do not rely on argv, SET has its own rules... */
    const char far *ptr;
    char buff[256];
    int i;
    /* locate the first space */
    for (ptr = cmdline; *ptr != ' '; ptr++);
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

    /* if not an = sign, then error */
    if (*ptr != '=') goto syntax_err;

    /* add the eq sign to buff */
    buff[i++] = '=';
    ptr++;

    /* copy value now, but make sure it contains no '=' sign */
    while (*ptr != '\r') {
      if (*ptr == '=') goto syntax_err;
      buff[i++] = *ptr;
      ptr++;
    }

    /* terminate buff */
    buff[i] = 0;

    /* TODO add it to environment */
    puts(buff);
    puts("TODO: ACTUALLY ADD TO ENV");

  }
  return(-1);

  syntax_err:

  puts("Syntax error");
  return(-1);
}
