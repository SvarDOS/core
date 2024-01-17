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
 * break
 */

static enum cmd_result cmd_break(struct cmd_funcparam *p) {
  unsigned char brkflag = 0;

  if (cmd_ishlp(p)) {
    nls_outputnl(14,0); /* "Sets or clears extended CTRL+C checking" */
    outputnl("");
    outputnl("BREAK [ON | OFF]");
    outputnl("");
    nls_outputnl(14,1); /* "Type BREAK without a parameter to display the current BREAK setting." */
    return(CMD_OK);
  }

  /* no params: display current break state */
  if (p->argc == 0) {
    _asm {
      push ax
      push dx

      mov ax, 0x3300   /* query break-check flag */
      int 0x21         /* status (0=OFF, 1=ON) in DL */
      mov [brkflag], dl

      pop dx
      pop ax
    }
    if (brkflag == 0) {
      nls_outputnl(14,2); /* "BREAK is off" */
    } else {
      nls_outputnl(14,3); /* "BREAK is on" */
    }
    return(CMD_OK);
  }

  /* too many params? */
  if (p->argc > 1) {
    nls_outputnl(0,4);
    return(CMD_FAIL);
  }

  /* exactly 1 parameter - "on" or "off" */
  if (imatch(p->argv[0], "on")) {
    brkflag = 1;
  } else if (!imatch(p->argv[0], "off")) {
    nls_outputnl(0,6); /* "Invalid parameter" */
    return(CMD_FAIL);
  }

  /* set break accordingly to brkflag */
  _asm {
    push ax
    push dx

    mov ax, 0x3301     /* set break-check level */
    mov dl, [brkflag]  /* 0=OFF 1=ON */
    int 0x21

    pop dx
    pop ax
  }

  return(CMD_OK);
}
