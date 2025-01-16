/* This file is part of the SvarCOM project and is published under the terms
 * of the MIT license.
 *
 * Copyright (C) 2021-2025 Mateusz Viste
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
 * prompt
 *
 * Changes the DOS command prompt.
 *
 */

static enum cmd_result cmd_prompt(struct cmd_funcparam *p) {

  if (cmd_ishlp(p)) {
    const char *s;
    unsigned short i;
    for (i = 0; i < 50; i++) {
      s = svarlang_str(33, i);
      if (*s != 0) outputnl(s);
    }
    return(CMD_OK);
  }

  /* no parameter - restore default prompt path */
  if (p->argc == 0) {
    env_dropvar(p->env_seg, "PROMPT");
    return(CMD_OK);
  }

  /* otherwise set PROMPT to whatever is passed on command-line */
  {
    unsigned short i;
    char *buff = p->BUFFER;
    sv_strcpy(buff, "PROMPT=");
    for (i = 0;; i++) {
      buff[i + 7] = p->cmdline[p->argoffset + i];
      if (buff[i + 7] == 0) break;
    }
    env_setvar(p->env_seg, buff);
  }

  return(CMD_OK);
}
