/* This file is part of the SvarCOM project and is published under the terms
 * of the MIT license.
 *
 * Copyright (C) 2021 Mateusz Viste
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
 * set [varname[=value]]
 *
 * value cannot contain any '=' character, but it can contain spaces
 * varname can also contain spaces
 */


static int cmd_set(struct cmd_funcparam *p) {
  char far *env = MK_FP(p->env_seg, 0);
  char *buff = p->BUFFER;

  if (cmd_ishlp(p)) {
    outputnl("Displays, sets, or removes DOS environment variables");
    outputnl("");
    outputnl("SET [variable=[string]]");
    outputnl("");
    outputnl("variable  Specifies the environment-variable name");
    outputnl("string    Specifies a series of characters to assign to the variable");
    outputnl("");
    outputnl("Type SET without parameters to display the current environment variables.");
  }

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
