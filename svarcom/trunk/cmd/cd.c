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
 * chdir
 *
 * displays the name of or changes the current directory.
 *
 * CHDIR [drive:][path]
 * CD..
 *
 * Type CD drive: to display the current directory in the specified drive.
 * Type CD without parameters to display the current drive and directory.
 */


static enum cmd_result cmd_cd(struct cmd_funcparam *p) {
  char *buffptr = p->BUFFER;

  /* CD /? */
  if (cmd_ishlp(p)) {
    outputnl("Displays the name of or changes the current directory.");
    outputnl("");
    outputnl("CHDIR [drive:][path]");
    outputnl("CHDIR[..]");
    outputnl("CD [drive:][path]");
    outputnl("CD[..]");
    outputnl("");
    outputnl(".. Specifies that you want to change to the parent directory.");
    outputnl("");
    outputnl("Type CD drive: to display the current directory in the specified drive.");
    outputnl("Type CD without parameters to display the current drive and directory.");
    return(CMD_OK);
  }

  /* one argument max */
  if (p->argc > 1) {
    outputnl("Too many parameters");
    return(CMD_FAIL);
  }

  /* no argument? display current drive and dir ("CWD") */
  if (p->argc == 0) {
    curpathfordrv(buffptr, 0);
    outputnl(buffptr);
    return(CMD_OK);
  }

  /* argument can be either a drive (D:) or a path */
  if (p->argc == 1) {
    const char *arg = p->argv[0];
    unsigned short err = 0;
    /* drive (CD B:) */
    if ((arg[0] != '\\') && (arg[1] == ':') && (arg[2] == 0)) {
      unsigned char drive = arg[0];
      if (drive >= 'a') {
        drive -= ('a' - 1);
      } else {
        drive -= ('A' - 1);
      }

      err = curpathfordrv(buffptr, drive);
      if (err == 0) outputnl(buffptr);
    } else { /* path */
      _asm {
        push dx
        push ax
        mov ah, 0x3B  /* CHDIR (set current directory) */
        mov dx, arg
        int 0x21
        jnc DONE
        mov [err], ax
        DONE:
        pop ax
        pop dx
      }
    }
    if (err != 0) {
      nls_outputnl_doserr(err);
      return(CMD_FAIL);
    }
  }

  return(CMD_OK);
}
