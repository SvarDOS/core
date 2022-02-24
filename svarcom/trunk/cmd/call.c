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
 * calls one batch program from another.
 *
 * CALL [drive:][path]filename [batch-parameters]
 *
 * batch-parameters    Specifies any command-line information required by the
 *                     batch program.
 */

static enum cmd_result cmd_call(struct cmd_funcparam *p) {
  if (cmd_ishlp(p)) {
    outputnl("Calls one batch program from another");
    outputnl("");
    outputnl("CALL [drive:][path]filename [batch-parameters]");
    return(CMD_OK);
  }

  /* no argument? do nothing */
  if (p->argc == 0) return(CMD_OK);

  /* change the command by moving batch filename and arguments to the start of the string */
  memmove((void *)(p->cmdline), p->cmdline + p->argoffset, strlen(p->cmdline + p->argoffset) + 1);

  return(CMD_CHANGED_BY_CALL); /* notify callee that command needs to be reevaluated */
}
