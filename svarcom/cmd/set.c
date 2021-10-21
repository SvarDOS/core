/*
 * set [varname[=value]]
 *
 * value cannot contain any '=' character, but it can contain spaces
 * varname can also contain spaces
 */

#include "env.h"


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
  } else { /* set variable (do not rely on argv, SET has its own rules...) */
    const char far *ptr;
    char buff[256];
    unsigned short i;
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

    /* copy value now */
    while (*ptr != '\r') {
      buff[i++] = *ptr;
      ptr++;
    }

    /* terminate buff */
    buff[i] = 0;

    /* commit variable to environment */
    i = env_setvar(env_seg, buff);
    if (i == ENV_INVSYNT) goto syntax_err;
    if (i == ENV_NOTENOM) puts("Not enough available space within the environment block");
  }
  return(-1);

  syntax_err:

  puts("Syntax error");
  return(-1);
}
