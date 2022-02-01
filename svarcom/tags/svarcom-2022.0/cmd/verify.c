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
 * verify
 */

static enum cmd_result cmd_verify(struct cmd_funcparam *p) {

  if (cmd_ishlp(p)) {
    outputnl("Tells DOS whether to verify that files are written correctly to disk.");
    outputnl("\r\nVERIFY [ON | OFF]\r\n");
    outputnl("Type VERIFY without a parameter to display its current setting.");
    return(CMD_OK);
  }

  if (p->argc > 1) {
    outputnl("Too many parameters");
    return(CMD_FAIL);
  }

  if (p->argc == 0) {
    unsigned char verstate = 0;
    _asm {
      push ax
      mov ah, 0x54   /* Get VERIFY status */
      int 0x21       /* AL == 0 (off) or AL == 1 (on) */
      mov [verstate], al
      pop ax
    }
    if (verstate == 0) {
      outputnl("VERIFY is off");
    } else {
      outputnl("VERIFY is on");
    }
    return(CMD_OK);
  }

  /* argc == 1*/
  if (imatch(p->argv[0], "on")) {
    _asm {
      push ax
      push dx
      mov ax, 0x2e01  /* set verify ON */
      xor dl, dl      /* apparently required by MS-DOS 2.x */
      int 0x21
      pop dx
      pop ax
    }
  } else if (imatch(p->argv[0], "off")) {
    _asm {
      push ax
      push dx
      mov ax, 0x2e00  /* set verify OFF */
      xor dl, dl      /* apparently required by MS-DOS 2.x */
      int 0x21
      pop dx
      pop ax
    }
  } else {
    outputnl("Must specify ON or OFF");
    return(CMD_FAIL);
  }

  return(CMD_OK);
}
