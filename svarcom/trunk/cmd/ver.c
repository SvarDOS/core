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
 * ver
 */

#define PVER "2021.0"

static int cmd_ver(struct cmd_funcparam *p) {
  char *buff = p->BUFFER;
  unsigned char maj = 0, min = 0;

  /* help screen */
  if (cmd_ishlp(p)) {
    outputnl("Displays the DOS version.");
    return(-1);
  }

  if (p->argc != 0) {
    outputnl("Invalid parameter");
    return(-1);
  }

  _asm {
    push ax
    push bx
    push cx
    mov ah, 0x30   /* Get DOS version number */
    int 0x21       /* AL=maj_ver_num  AH=min_ver_num  BX,CX=OEM */
    mov [maj], al
    mov [min], ah
    pop cx
    pop bx
    pop ax
  }

  sprintf(buff, "DOS kernel version %u.%u", maj, min);

  outputnl(buff);
  outputnl("SvarCOM shell ver " PVER);
  return(-1);
}
