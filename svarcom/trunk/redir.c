/* This file is part of the SvarCOM project and is published under the terms
 * of the MIT license.
 *
 * Copyright (C) 2021-2026 Mateusz Viste
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

#include "env.h"
#include "helpers.h"
#include "rmodinit.h"

#include "redir.h"

static unsigned short oldstdout = 0xffff;
static unsigned short oldstdin = 0xffff;


/* compute a filename to be used for pipes */
static unsigned short gentmpfile(char *s, unsigned short envseg) {
  unsigned short err = 0;
  unsigned short i;

  /* do I have a %temp% path? */
  i = env_lookup_valcopy(s, 116, envseg, "TEMP");
  if (i > 0) {
    /* make sure it is terminated by a backslash (required by int 0x21, ah=5a) */
    if (s[i - 1] != '\\') {
      s[i++] = '\\';
      s[i] = 0;
    }
  } else {
    /* if fails, then use truename(.\) */
    if (file_truename(".\\", s) != 0) *s = 0;
  }

  /* create file */
  _asm {
    mov ah, 0x5a
    mov dx, s
    xor cx, cx /* file attributes */
    int 0x21
    jnc CLOSEFILE
    mov err, ax
    jmp DONE
    /* close file handle (handle still in BX) */
    CLOSEFILE:
    mov bx, ax
    mov ah, 0x3e
    int 0x21
    DONE:
  }
  return(err);
}


/* append s to dst if it fits in dst[bufsz] (bufsz includes the terminating NUL
 * written by sv_strcat_far). len is the current length. returns 0 on success. */
static int cat_if_fits(char far *dst, unsigned short *len, unsigned short bufsz, const char *s) {
  unsigned short need = sv_strlen(s);
  if ((*len >= bufsz) || (need >= bufsz - *len)) return(-1);
  sv_strcat_far(dst, s);
  *len += need;
  return(0);
}


/* the pending pipe command does not fit. drop whatever was queued. */
static unsigned short pipe_wont_fit(char far *awaitingcmd) {
  *awaitingcmd = 0;
  nls_outputnl(18, 8); /* "Line too long" */
  return(REDIR_ERR_TOOLONG);
}


/* parse commandline and performs necessary redirections. cmdline is
 * modified so all redirections are cut out.
 * piped commands are moved to awaitingcmd for later execution
 * returns 0 on success, a DOS error on failure, or REDIR_ERR_TOOLONG */
unsigned short redir_parsecmd(struct redir_data *d, char *cmdline, char far *awaitingcmd, unsigned short envseg) {
  unsigned short i;
  unsigned short pipescount = 0;

  /* NOTES:
   *
   * 1. while it is possible to type a command with multiple
   *    redirections, MSDOS executes only the last redirection.
   *
   * 2. the order in which >, < and | are provided on the command line does
   *    not seem to matter for MSDOS. piped commands are executed first (in
   *    the order they appear) and the result of the last one is redirected to
   *    whenever the last > points at.
   *    stdin redirection (<) is (obviously) applied to the first command only
   */

  /* preset to 0xffff in case no redirection is required */
  oldstdout = 0xffff;
  oldstdin = 0xffff;

  /* clear out the redir_data struct */
  sv_bzero(d, sizeof(*d));

  *awaitingcmd = 0;

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

  /* if pipes present, write them to awaitingcmd (and stdout redirection too).
   * awaitingcmd is followed by the bat and forloop pointers, so a queue that
   * does not fit must be rejected rather than truncated. */
  if (pipescount != 0) {
    static char tmpfile[130];
    unsigned short len = 0;

    for (i = 0; i < pipescount; i++) {
      if (i != 0) {
        if (cat_if_fits(awaitingcmd, &len, RMOD_AWAITINGCMD_SZ, "|") != 0) return(pipe_wont_fit(awaitingcmd));
      }
      if (cat_if_fits(awaitingcmd, &len, RMOD_AWAITINGCMD_SZ, d->pipes[i]) != 0) return(pipe_wont_fit(awaitingcmd));
    }
    /* append stdout redirection so I don't forget about it for the last command of the pipe queue */
    if (d->stdoutfile != NULL) {
      if (d->stdout_openflag == 0x11) {
        if (cat_if_fits(awaitingcmd, &len, RMOD_AWAITINGCMD_SZ, ">>") != 0) return(pipe_wont_fit(awaitingcmd));
      } else {
        if (cat_if_fits(awaitingcmd, &len, RMOD_AWAITINGCMD_SZ, ">") != 0) return(pipe_wont_fit(awaitingcmd));
      }
      /* operator alone is not enough: "dir | sort >out.txt" must keep out.txt */
      if (cat_if_fits(awaitingcmd, &len, RMOD_AWAITINGCMD_SZ, d->stdoutfile) != 0) return(pipe_wont_fit(awaitingcmd));
      d->stdoutfile = NULL;
    }
    /* redirect stdin of next command from a temp file (that is used as my output) */
    if (cat_if_fits(awaitingcmd, &len, RMOD_AWAITINGCMD_SZ, "<") != 0) return(pipe_wont_fit(awaitingcmd));
    i = gentmpfile(tmpfile, envseg);
    if (i != 0) return(i);
    if (cat_if_fits(awaitingcmd, &len, RMOD_AWAITINGCMD_SZ, tmpfile) != 0) {
      /* the temp file will not be consumed */
      _asm {
        push ax
        push bx
        push cx
        push dx
        push si
        mov ah, 0x41
        lea dx, tmpfile   /* static array: DX needs its address, not the first byte */
        int 0x21
        pop si
        pop dx
        pop cx
        pop bx
        pop ax
      }
      return(pipe_wont_fit(awaitingcmd));
    }
    /* same file is used as my stdout */
    d->stdoutfile = tmpfile;
    d->stdout_openflag = 0x12;
  }
  return(0);
}


/* apply stdin/stdout redirections defined in redir_data, returns 0 on success */
int redir_apply(const struct redir_data *d) {

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
      mov handle, ax     /* save the file handler */
      jnc JMPEOF
      mov errcode, ax
      jmp DONE

      JMPEOF:
      cmp openflag, word ptr 0x11
      jne DUPSTDOUT
      /* jump to the end of the file (required for >> redirections) */
      mov ax, 0x4202     /* jump to position EOF - CX:DX in handle BX */
      mov bx, handle
      xor cx, cx
      xor dx, dx
      int 0x21

      /* save (duplicate) current stdout so I can revert it later */
      DUPSTDOUT:
      mov ah, 0x45       /* duplicate file handle */
      mov bx, 1          /* handle to dup (1=stdout) */
      int 0x21           /* ax = new file handle */
      mov oldstdout, ax
      /* redirect the stdout handle */
      mov bx, handle     /* dst handle */
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
      nls_outputnl_doserr(errcode);
      return(-1);
    }
  }

  /* stdin (<). external programs get this from rmod; internal commands
   * (DATE, TIME) read the standard input themselves. */
  if (d->stdinfile != NULL) {
    unsigned short errcode = 0;
    unsigned short handle = 0;
    const char *myptr = d->stdinfile;

    _asm {
      push ax
      push bx
      push cx
      push dx
      push si
      mov ax, 0x3d00     /* open for read */
      mov dx, myptr      /* ASCIIZ filename */
      int 0x21           /* AX=handle on success (CF clear), otherwise dos err */
      mov handle, ax
      jnc STDIN_OPENED
      mov errcode, ax
      jmp STDIN_DONE

      /* save (duplicate) current stdin so I can revert it later */
      STDIN_OPENED:
      mov ah, 0x45       /* duplicate file handle */
      xor bx, bx         /* handle to dup (0=stdin) */
      int 0x21           /* ax = new file handle */
      mov oldstdin, ax
      /* redirect the stdin handle */
      mov bx, handle     /* opened file */
      xor cx, cx         /* handle to replace (0=stdin) */
      mov ah, 0x46       /* redirect a handle */
      int 0x21
      /* close the original file handle (no longer needed) */
      mov ah, 0x3e       /* close a file handle (handle in BX) */
      int 0x21
      STDIN_DONE:
      pop si
      pop dx
      pop cx
      pop bx
      pop ax
    }
    if (errcode != 0) {
      /* stdout may already point at another file: restore it first,
       * otherwise the error message itself is redirected away */
      redir_revert();
      nls_outputnl_doserr(errcode);
      return(-1);
    }
  }

  return(0);
}


/* restores previous stdout/stdin handles if they have been redirected */
void redir_revert(void) {
  _asm {
    /* if oldstdout is 0xffff then not redirected */
    mov bx, [oldstdout] /* dst handle */
    cmp bx, 0xffff
    je STDOUT_DONE
    /* redirect the stdout handle (handle already in BX) */
    mov cx, 1           /* src handle (1=stdout) */
    mov ah, 0x46        /* redirect a handle */
    int 0x21
    /* close old handle (in bx already) */
    mov ah, 0x3e
    int 0x21
    mov [oldstdout], 0xffff
    STDOUT_DONE:

    /* if oldstdin is 0xffff then not redirected */
    mov bx, [oldstdin]
    cmp bx, 0xffff
    je STDIN_DONE
    /* redirect the stdin handle (handle already in BX) */
    xor cx, cx          /* src handle (0=stdin) */
    mov ah, 0x46        /* redirect a handle */
    int 0x21
    /* close old handle (in bx already) */
    mov ah, 0x3e
    int 0x21
    mov [oldstdin], 0xffff
    STDIN_DONE:
  }
}


/* point the handles saved by redir_apply() at devhandle, so redir_revert()
 * restores that device instead of the previous console. devhandle is not
 * closed. returns 0 on success. */
unsigned short redir_point_saved(unsigned short devhandle) {
  unsigned short errcode = 0;

  _asm {
    push ax
    push bx
    push cx

    /* AH=46h: BX = existing handle, CX = handle to replace */
    mov bx, [devhandle]
    mov cx, [oldstdout]
    cmp cx, 0xffff
    je SAVED_STDOUT_DONE
    cmp cx, bx
    je SAVED_STDOUT_DONE
    mov ah, 0x46
    int 0x21
    jnc SAVED_STDOUT_DONE
    mov [errcode], ax
    SAVED_STDOUT_DONE:

    mov bx, [devhandle]
    mov cx, [oldstdin]
    cmp cx, 0xffff
    je SAVED_STDIN_DONE
    cmp cx, bx
    je SAVED_STDIN_DONE
    mov ah, 0x46
    int 0x21
    jnc SAVED_STDIN_DONE
    mov [errcode], ax

    SAVED_STDIN_DONE:
    pop cx
    pop bx
    pop ax
  }

  return(errcode);
}
