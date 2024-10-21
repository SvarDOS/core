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
 * truename [path]
 */

static enum cmd_result cmd_truename(struct cmd_funcparam *p) {
  unsigned short res;

  if (cmd_ishlp(p)) {
    nls_outputnl(39,0);
    outputnl("");
    nls_outputnl(39,1);
    return(CMD_OK);
  }

  if (p->argc > 1) {
    nls_outputnl(0,4); /* too many parameters */
    return(CMD_FAIL);
  }

  if (p->argc == 0) {
    res = file_truename(".", p->BUFFER);
  } else {
    res = file_truename(p->argv[0], p->BUFFER);
  }

  if (res != 0) {
    nls_outputnl_doserr(res);
    return(CMD_FAIL);
  }

  outputnl(p->BUFFER);

  return(CMD_OK);
}
