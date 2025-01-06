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
 * cls
 */

static enum cmd_result cmd_cls(struct cmd_funcparam *p) {
  unsigned char screenw, screenh;
  const char *ansiesc = "\x1B[2J$";

  if (cmd_ishlp(p)) {
    nls_outputnl(10,0); /* "Clears the screen" */
    outputnl("");
    outputnl("CLS");
    return(CMD_OK);
  }

  screenw = screen_getwidth();
  screenh = screen_getheight();

  _asm {
    /* output an ANSI ESC code for "clear screen" in case the console is
     * some kind of terminal */
    mov ah, 0x09     /* write $-terminated string to stdout */
    mov dx, ansiesc
    int 0x21

    /* check what stdout is set to */
    mov ax, 0x4400   /* IOCTL query device/flags flags */
    mov bx, 1        /* file handle (1 = stdout) */
    int 0x21         /* CF set on error, otherwise DX set with flags */
    jc DONE          /* abort on error */
    /* DX = 10000010
            |   |||
            |   ||+--- indicates standard output
            |   |+---- set if NUL device
            |   +----- set if CLOCK device
            +--------- set if handle is a device (ie. not a file)
            in other words, DL & 10001110 (8Eh) should result in 10000010 (82h)
            */
    and dl, 0x8e
    cmp dl, 0x82
    jne DONE         /* abort on error */

    /* scroll vram out of screen */
    mov ax, 0x0600   /* scroll up entire rectangle */
    mov bh, 0x07     /* fill screen with white-on-black */
    xor cx, cx       /* upper left location in CH,CL (0,0) */
    /* DX is bottom right location of rectangle (DH=row, DL=column) */
    mov dh, [screenh]
    dec dh
    mov dl, [screenw]
    dec dl
    int 0x10

    /* set cursor to top left corner (0,0) of the screen */
    mov ah, 0x02     /* set cursor position */
    xor bh, bh       /* page number */
    xor dx, dx       /* location in DH,DL */
    int 0x10
    DONE:
  }

  return(CMD_OK);
}
