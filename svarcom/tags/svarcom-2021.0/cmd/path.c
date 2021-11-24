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
 * path
 *
 * Displays or sets a search path for executable files.
 */

static int cmd_path(struct cmd_funcparam *p) {
  char *buff = p->BUFFER;

  /* help screen (/?) */
  if (cmd_ishlp(p)) {
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
      if (buff[i + 5] == 0) break;
    }
    env_setvar(p->env_seg, buff);
  }

  return(-1);
}
