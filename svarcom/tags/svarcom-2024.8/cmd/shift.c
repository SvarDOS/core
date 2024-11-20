/* This file is part of the SvarCOM project and is published under the terms
 * of the MIT license.
 *
 * Copyright (C) 2021-2024 Mateusz Viste
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
 * shift
 */

static enum cmd_result cmd_shift(struct cmd_funcparam *p) {
  char far *batargv;
  char far *nextarg;

  if (cmd_ishlp(p)) {
    nls_outputnl(16, 0);
    nls_outputnl(16, 1);
    outputnl("");
    outputnl("SHIFT");
    return(CMD_OK);
  }

  /* abort if batargv is empty */
  if ((p->rmod->bat == NULL) || (p->rmod->bat->argv[0] == 0)) return(CMD_OK);
  batargv = p->rmod->bat->argv;

  /* find the next argument in batargv */
  for (nextarg = batargv + 1; *nextarg != 0; nextarg++);
  nextarg++; /* move ptr past the zero terminator */

  /* move down batargv so 2nd argument is at the head now */
  memcpy_ltr_far(batargv, nextarg, sizeof(p->rmod->bat->argv) - (nextarg - batargv));

  return(CMD_OK);
}
