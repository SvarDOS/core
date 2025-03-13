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
 * loadhigh
 */

static enum cmd_result cmd_loadhigh(struct cmd_funcparam *p) {

  if ((p->argc == 0) || (imatch(p->argv[0], "/?"))) {
    nls_outputnl(40,0);
    outputnl("");
    output("LOADHIGH ");
    nls_outputnl(40,1);
    output("LH ");
    nls_outputnl(40,1);
    outputnl("");
    nls_outputnl(40,2);
    return(CMD_OK);
  }

  /* set the command to be executed */
  if (p->cmdline[p->argoffset] != 0) {
    memcpy_ltr((void *)p->cmdline, p->cmdline + p->argoffset, sv_strlen(p->cmdline + p->argoffset) + 1);
  }

  return(CMD_CHANGED_BY_LH);
}
