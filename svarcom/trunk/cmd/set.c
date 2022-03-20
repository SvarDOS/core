/* This file is part of the SvarCOM project and is published under the terms
 * of the MIT license.
 *
 * Copyright (C) 2021-2022 Mateusz Viste
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


static enum cmd_result cmd_set(struct cmd_funcparam *p) {
  char far *env = MK_FP(p->env_seg, 0);
  char *buff = p->BUFFER;

  if (cmd_ishlp(p)) {
    nls_outputnl(23,0); /* "Displays, sets, or removes DOS environment variables"); */
    outputnl("");
    nls_outputnl(23,1); /* "SET [variable=[string]]" */
    outputnl("");
    nls_outputnl(23,2); /* "variable  Specifies the environment-variable name" */
    nls_outputnl(23,3); /* "string    Specifies a series of characters to assign to the variable" */
    outputnl("");
    nls_outputnl(23,4); /* "Type SET without parameters to display the current environment variables." */
    return(CMD_OK);
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

    /* locate the first space (note that cmdline separators should be sanitized
     * to space only by now) */
    for (ptr = p->cmdline; *ptr != ' '; ptr++);

    /* now locate the first non-space: that's where the variable name begins */
    for (; *ptr == ' '; ptr++);

    /* copy variable name to buff */
    i = 0;
    for (; *ptr != '='; ptr++) {
      if (*ptr == 0) goto syntax_err;
      buff[i++] = *ptr;
    }

    /* make variable name all caps (country-dependend) */
    buff[i] = 0;
    nls_strtoup(buff);

    /* copy value now */
    while (*ptr != 0) {
      buff[i++] = *ptr;
      ptr++;
    }

    /* terminate buff */
    buff[i] = 0;

    /* commit variable to environment */
    i = env_setvar(p->env_seg, buff);
    if (i == ENV_INVSYNT) goto syntax_err;
    if (i == ENV_NOTENOM) {
      nls_outputnl(23,5); /* "Not enough available space within the environment block" */
      return(CMD_FAIL);
    }
  }
  return(CMD_OK);

  syntax_err:

  nls_outputnl(0,1); /* "Invalid syntax" */
  return(CMD_FAIL);
}
