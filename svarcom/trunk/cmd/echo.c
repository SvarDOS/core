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
 * echo
 */

static int cmd_echo(struct cmd_funcparam *p) {
  unsigned short offs = FP_OFF(p->cmdline) + 5;
  unsigned short segm = FP_SEG(p->cmdline);

  /* display help only if /? is the only argument */
  if ((p->argc == 1) && (imatch(p->argv[0], "/?"))) {
    outputnl("Displays messages, or turns command-echoing on or off");
    outputnl("");
    outputnl("ECHO [ON | OFF]");
    outputnl("ECHO [message]");
    outputnl("");
    outputnl("Type ECHO without parameters to display the current echo setting.");
    return(-1);
  }

  /* ECHO without any parameter: display current state */
  if (p->argc == 0) {
    if (p->rmod->echoflag) {
      outputnl("ECHO is on");
    } else {
      outputnl("ECHO is off");
    }
    return(-1);
  }

  /* ECHO ON */
  if ((p->argc == 1) && (imatch(p->argv[0], "on"))) {
    p->rmod->echoflag = 1;
    return(-1);
  }

  /* ECHO OFF */
  if ((p->argc == 1) && (imatch(p->argv[0], "off"))) {
    p->rmod->echoflag = 0;
    return(-1);
  }

  /* ECHO MSG (start at cmdline+5 since first 5 are "ECHO" + separator) */
  _asm {
    push ax
    push dx
    push ds
    push si

    mov si, [offs]
    cld           /* clear direction flag (DF) so lodsb increments SI */
    mov ah, 0x02  /* display char from DL */
    mov ds, [segm]
    NEXTYBTE:
    lodsb         /* load byte at DS:[SI] into AL and inc SI (if DF clear) */
    or al, al     /* is AL == 0? then end of string reached */
    jz DONE
    mov dl, al
    int 0x21
    jmp NEXTYBTE

    /* output a final CR/LF */
    DONE:
    mov dl, 0x0D
    int 0x21
    mov dl, 0x0A
    int 0x21

    pop si
    pop ds
    pop dx
    pop ax
  }

  return(-1);
}