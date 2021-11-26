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
 * chcp
 */

static enum cmd_result cmd_chcp(struct cmd_funcparam *p) {
  unsigned short nnn = 0;
  unsigned short errcode = 0;

  if (cmd_ishlp(p)) {
    outputnl("Displays or sets the active code page number");
    outputnl("");
    outputnl("CHCP [nnn]");
    outputnl("");
    outputnl("nnn  Specifies a code page number");
    outputnl("");
    outputnl("Type CHCP without a parameter to display the active code page number.");
    return(CMD_OK);
  }

  /* too many parameters */
  if (p->argc > 1) {
    outputnl("Too many parameters");
    return(CMD_FAIL);
  }

  /* one param? must be numeric in range 1+ */
  if (p->argc == 1) {
    unsigned char nlsfuncflag = 0;
    if (atous(&nnn, p->argv[0]) != 0) {
      outputnl("Invalid code page number");
      return(CMD_FAIL);
    }
    _asm {
      /* verify that NLSFUNC is installed */
      push ax
      push bx

      mov ax, 0x1400    /* DOS 3+ -- is NLSFUNC.EXE installed? */
      int 0x2f          /* AL = 0xff -> installed */
      cmp al, 0xff
      jne DONE
      mov [nlsfuncflag], 1

      /* set code page to nnn */

      mov ax, 0x6602    /* DOS 3.3+ -- Activate Code Page */
      mov bx, [nnn]
      int 0x21          /* CF set on error and err code in AX */
      jnc DONE
      mov [errcode], ax /* store err code in nnn on failure */
      DONE:

      pop bx
      pop ax
    }
    if (nlsfuncflag == 0) {
      outputnl("NLSFUNC not installed");
    } else if (errcode != 0) {
      outputnl("Failed to change code page");
      return(CMD_FAIL);
    }

  } else { /* no parameter given: display active code page */

    _asm {
      push ax
      push bx
      push dx

      mov ax, 0x6601      /* DOS 3.3+ -- Query Active Code Page */
      int 0x21            /* CF set on error, current CP in BX */
      mov [nnn], bx
      jnc DONE
      mov [errcode], ax
      DONE:

      pop dx
      pop bx
      pop ax
    }
    if (errcode == 0) {
      sprintf(p->BUFFER, "Active code page: %d", nnn);
      outputnl(p->BUFFER);
    } else {
      outputnl(doserr(errcode));
      return(CMD_FAIL);
    }
  }

  return(CMD_OK);
}
