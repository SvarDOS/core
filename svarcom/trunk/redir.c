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

#include <string.h> /* memset() */

#include "doserr.h"
#include "helpers.h"
#include "rmodinit.h"

#include "redir.h"

static unsigned short oldstdout = 0xffff;


/* parse commandline and performs necessary redirections. cmdline is
 * modified so all redirections are cut out. */
void redir_parsecmd(struct redir_data *d, char *cmdline) {
  unsigned short i;
  unsigned short pipescount = 0;

  /* NOTE:
   *
   * 1. while it is possible to type a command with multiple
   *    redirections, MSDOS executes only the last redirection.
   *
   * 2. the order in which >, < and | are provided on the command line does
   *    not seem to matter for MSDOS. piped commands are executed first (in
   *    the order they appear) and the result of the last one is redirected to
   *    whenever the last > points at.
   */

  /* preset oldstdout to 0xffff in case no redirection is required */
  oldstdout = 0xffff;

  /* clear out the redir_data struct */
  memset(d, 0, sizeof(*d));

  /* parse the command line and fill struct with pointers */
  for (i = 0;; i++) {
    if (cmdline[i] == '>') {
      cmdline[i] = 0;
      if (cmdline[i + 1] == '>') {
        i++;
        d->stdout_openflag = 0x11;  /* used during int 21h,AH=6C */
      } else {
        d->stdout_openflag = 0x12;
      }
      d->stdoutfile = cmdline + i + 1;
      while (d->stdoutfile[0] == ' ') d->stdoutfile++;
    } else if (cmdline[i] == '<') {
      cmdline[i] = 0;
      d->stdinfile = cmdline + i + 1;
      while (d->stdinfile[0] == ' ') d->stdinfile++;
    } else if (cmdline[i] == '|') {
      cmdline[i] = 0;
      if (pipescount < REDIR_MAX_PIPES) {
        d->pipes[pipescount++] = cmdline + i + 1;
        while (d->pipes[pipescount][0] == ' ') d->pipes[pipescount]++;
      }
    } else if (cmdline[i] == 0) {
      break;
    }
  }
}


/* apply stdin/stdout redirections defined in redir_data, returns 0 on success */
int redir_apply(const struct redir_data *d) {
  if (d->stdinfile != NULL) {
    outputnl("ERROR: stdin redirection is not supported yet");
    return(-1);
  }

  if (d->stdoutfile != NULL) {
    unsigned short openflag = d->stdout_openflag;
    unsigned short errcode = 0;
    unsigned short handle = 0;
    const char *myptr = d->stdoutfile;

    /* */
    _asm {
      push ax
      push bx
      push cx
      push dx
      push si
      mov ax, 0x6c00     /* Extended Open/Create */
      mov bx, 1          /* access mode (0=read, 1=write, 2=r+w */
      xor cx, cx         /* attributes when(if) creating the file (0=normal) */
      mov dx, [openflag] /* action if file exists (0x11=open, 0x12=truncate)*/
      mov si, myptr      /* ASCIIZ filename */
      int 0x21           /* AX=handle on success (CF clear), otherwise dos err */
      mov [handle], ax   /* save the file handler */
      jnc DUPSTDOUT
      mov [errcode], ax
      jmp DONE
      DUPSTDOUT:
      /* save (duplicate) current stdout so I can revert it later */
      mov ah, 0x45       /* duplicate file handle */
      mov bx, 1          /* handle to dup (1=stdout) */
      int 0x21           /* ax = new file handle */
      mov [oldstdout], ax
      /* redirect the stdout handle */
      mov bx, [handle]   /* dst handle */
      mov cx, 1          /* src handle (1=stdout) */
      mov ah, 0x46       /* redirect a handle */
      int 0x21
      /* close the original file handle (no longer needed) */
      mov ah, 0x3e       /* close a file handle (handle in BX) */
      int 0x21
      DONE:
      pop si
      pop dx
      pop cx
      pop bx
      pop ax
    }
    if (errcode != 0) {
      outputnl(doserr(errcode));
      return(-1);
    }
  }

  return(0);
}


/* restores previous stdout/stdin handlers if they have been redirected */
void redir_revert(void) {
  _asm {
    /* if oldstdout is 0xffff then not redirected */
    cmp word ptr [oldstdout], 0xffff
    je DONE
    /* redirect the stdout handle */
    mov bx, [oldstdout] /* dst handle */
    mov cx, 1           /* src handle (1=stdout) */
    mov ah, 0x46        /* redirect a handle */
    int 0x21
    /* close old handle (in bx already) */
    mov ah, 0x3e
    int 0x21
    mov [oldstdout], 0xffff
    DONE:
  }
}
