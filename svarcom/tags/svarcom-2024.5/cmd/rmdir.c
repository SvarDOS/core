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
 * rmdir
 */

static enum cmd_result cmd_rmdir(struct cmd_funcparam *p) {
  const char *dname = p->argv[0];
  unsigned short err = 0;

  if (cmd_ishlp(p)) {
    nls_outputnl(24,0); /* "Removes (deletes) a directory" */
    outputnl("");
    nls_outputnl(24,1);/* "RMDIR [drive:]path" */
    nls_outputnl(24,2);/* "RD [drive:]path" */
    return(CMD_OK);
  }

  if (p->argc == 0) {
    nls_outputnl(0,7); /* "Required parameter missing" */
    return(CMD_FAIL);
  }

  if (p->argc > 1) {
    nls_outputnl(0,4); /* "Too many parameters" */
    return(CMD_FAIL);
  }

  if (p->argv[0][0] == '/') {
    nls_outputnl(0,6); /* "Invalid parameter"); */
    return(CMD_FAIL);
  }

  _asm {
    push ax
    push dx

    mov ah, 0x3a   /* delete a directory, DS:DX points to ASCIIZ dir name */
    mov dx, [dname]
    int 0x21
    jnc DONE
    mov [err], ax
    DONE:

    pop dx
    pop ax
  }

  if (err != 0) {
    nls_outputnl_doserr(err);
    return(CMD_FAIL);
  }

  return(CMD_OK);
}
