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

#ifndef REDIR_H
#define REDIR_H

#define REDIR_MAX_PIPES 15

struct redir_data {
  char *pipes[REDIR_MAX_PIPES + 1];
  char *stdinfile;
  char *stdoutfile;
  unsigned short stdout_openflag; /* 0x11 or 0x12, used for the 'extended open' call */
};

/* parse commandline and performs necessary redirections. cmdline is
 * modified so all redirections are cut out. */
void redir_parsecmd(struct redir_data *r, char *cmdline);

/* apply stdin/stdout redirections defined in redir_data, returns 0 on success */
int redir_apply(const struct redir_data *d);

/* restores previous stdout/stdin handlers if they have been redirected */
void redir_revert(void);

#endif
