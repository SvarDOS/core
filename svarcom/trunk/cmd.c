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

/* entry point for internal commands
 * matches internal commands and executes them
 * returns -1 or exit code if processed
 * returns -2 if command unrecognized
 */

#include <i86.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "doserr.h"
#include "env.h"
#include "helpers.h"
#include "redir.h"
#include "rmodinit.h"
#include "sayonara.h"

#include "cmd.h"


struct cmd_funcparam {
  int argc;                 /* number of arguments */
  const char *argv[128];    /* pointers to each argument */
  char argvbuf[256];        /* buffer that hold data pointed out by argv[] */
  unsigned short env_seg;   /* segment of environment block */
  struct rmod_props far *rmod; /* rmod settings */
  unsigned short argoffset; /* offset of cmdline where first argument starts */
  const char *cmdline;      /* original cmdline (terminated by a NULL) */
  unsigned short BUFFERSZ;  /* avail space in BUFFER */
  char BUFFER[1];           /* a buffer for whatever is needed (must be last) */
};

/* scans argv for the presence of a "/?" parameter. returns 1 if found, 0 otherwise */
static int cmd_ishlp(const struct cmd_funcparam *p) {
  int i;
  for (i = 0; i < p->argc; i++) {
    if ((p->argv[i][0] == '/') && (p->argv[i][1] == '?')) return(1);
  }
  return(0);
}

#include "cmd/_notimpl.c"
#include "cmd/break.c"
#include "cmd/cd.c"
#include "cmd/chcp.c"
#include "cmd/cls.c"
#include "cmd/copy.c"
#include "cmd/date.c"
#include "cmd/del.c"
#include "cmd/if.c"
#include "cmd/vol.c"     /* must be included before dir.c due to dependency */
#include "cmd/dir.c"
#include "cmd/echo.c"
#include "cmd/exit.c"
#include "cmd/mkdir.c"
#include "cmd/path.c"
#include "cmd/pause.c"
#include "cmd/prompt.c"
#include "cmd/rem.c"
#include "cmd/rename.c"
#include "cmd/rmdir.c"
#include "cmd/set.c"
#include "cmd/shift.c"
#include "cmd/time.c"
#include "cmd/type.c"
#include "cmd/ver.c"
#include "cmd/verify.c"


struct CMD_ID {
  const char *cmd;
  enum cmd_result (*func_ptr)(struct cmd_funcparam *); /* pointer to handling function */
};

const struct CMD_ID INTERNAL_CMDS[] = {
  {"BREAK",   cmd_break},
  {"CD",      cmd_cd},
  {"CHCP",    cmd_chcp},
  {"CHDIR",   cmd_cd},
  {"CLS",     cmd_cls},
  {"COPY",    cmd_copy},
  {"CTTY",    cmd_notimpl},
  {"DATE",    cmd_date},
  {"DEL",     cmd_del},
  {"DIR",     cmd_dir},
  {"ECHO",    cmd_echo},
  {"ERASE",   cmd_del},
  {"EXIT",    cmd_exit},
  {"IF",      cmd_if},
  {"LH",      cmd_notimpl},
  {"LOADHIGH",cmd_notimpl},
  {"MD",      cmd_mkdir},
  {"MKDIR",   cmd_mkdir},
  {"PAUSE",   cmd_pause},
  {"PATH",    cmd_path},
  {"PROMPT",  cmd_prompt},
  {"RD",      cmd_rmdir},
  {"REM",     cmd_rem},
  {"REN",     cmd_rename},
  {"RENAME",  cmd_rename},
  {"RMDIR",   cmd_rmdir},
  {"SET",     cmd_set},
  {"SHIFT",   cmd_shift},
  {"TIME",    cmd_time},
  {"TYPE",    cmd_type},
  {"VER",     cmd_ver},
  {"VERIFY",  cmd_verify},
  {"VOL",     cmd_vol},
  {NULL,      NULL}
};


/* NULL if cmdline is not matching an internal command, otherwise returns a
 * pointer to a CMD_ID struct */
static const struct CMD_ID *cmd_match(const char *cmdline, unsigned short *argoffset) {
  unsigned short i;
  char buff[10];

  /* copy command to buffer, until space, NULL, tab, return, dot, slash or backslash */
  for (i = 0; i < 9; i++) {
    if (cmdline[i] == ' ') break;
    if (cmdline[i] == 0) break;
    if (cmdline[i] == '\t') break;
    if (cmdline[i] == '\r') break;
    if (cmdline[i] == '.') break;
    if (cmdline[i] == '/') break;
    if (cmdline[i] == '\\') break;
    buff[i] = cmdline[i];
  }
  buff[i] = 0;

  /* advance to nearest non-space to find where arguments start */
  while (cmdline[i] == ' ') i++;
  *argoffset = i;

  /* try matching an internal command */
  for (i = 0; INTERNAL_CMDS[i].cmd != NULL; i++) {
    /*printf("imatch(%s,%s)\r\n", buff, INTERNAL_CMDS[i].cmd); */
    if (imatch(buff, INTERNAL_CMDS[i].cmd)) {
      /*printf("match cmd i=%u (buff=%s)\r\n", i, buff);*/
      return(&(INTERNAL_CMDS[i]));
    }
  }

  return(NULL); /* command is not recognized as internal */
}


/* explodes a command into an array of arguments where last arg is NULL.
 * if argvlist is not NULL, it will be filled with pointers that point to buff
 * locations. buff is filled with all the arguments, each argument being
 * zero-separated. buff is terminated with an empty argument to mark the end
 * of arguments.
 * returns number of args */
unsigned short cmd_explode(char *buff, const char far *s, char const **argvlist) {
  int si = 0, argc = 0, i = 0;
  for (;;) {
    /* skip to next non-space character */
    while (s[si] == ' ') si++;
    /* end of string? */
    if (s[si] == 0) break;
    /* set argv ptr */
    if (argvlist) argvlist[argc] = buff + i;
    argc++;
    /* find next arg delimiter (spc, null, slash or plus) while copying arg to local buffer */
    do {
      buff[i++] = s[si++];
    } while (s[si] != ' ' && s[si] != 0 && s[si] != '/' && s[si] != '+');
    buff[i++] = 0;
    /* is this end of string? */
    if (s[si] == 0) break;
  }
  buff[i] = 0; /* terminate with one extra zero to tell "this is the end of list" */
  if (argvlist) argvlist[argc] = NULL;
  return(argc);
}


enum cmd_result cmd_process(struct rmod_props far *rmod, unsigned short env_seg, const char *cmdline, void *BUFFER, unsigned short BUFFERSZ, const struct redir_data *redir) {
  const struct CMD_ID *cmdptr;
  unsigned short argoffset;
  enum cmd_result cmdres;
  struct cmd_funcparam *p = (void *)BUFFER;
  p->BUFFERSZ = BUFFERSZ - sizeof(*p);

  /* special case: is this a drive change? (like "E:") */
  if ((cmdline[0] != 0) && (cmdline[1] == ':') && ((cmdline[2] == ' ') || (cmdline[2] == 0))) {
    if (((cmdline[0] >= 'a') && (cmdline[0] <= 'z')) || ((cmdline[0] >= 'A') && (cmdline[0] <= 'Z'))) {
      unsigned char drive = cmdline[0];
      unsigned char curdrive = 0;
      if (drive >= 'a') {
        drive -= 'a';
      } else {
        drive -= 'A';
      }
      _asm {
        push ax
        push dx
        mov ah, 0x0e     /* DOS 1+ - SELECT DEFAULT DRIVE */
        mov dl, drive    /* DL = new default drive (00h = A:, 01h = B:, etc) */
        int 0x21
        mov ah, 0x19     /* DOS 1+ - GET CURRENT DRIVE */
        int 0x21
        mov curdrive, al /* cur drive (0=A, 1=B, etc) */
        pop dx
        pop ax
      }
      if (curdrive == drive) return(CMD_OK);
      outputnl(doserr(0x0f));
      return(CMD_FAIL);
    }
  }

  /* try matching an internal command */
  cmdptr = cmd_match(cmdline, &argoffset);
  if (cmdptr == NULL) return(CMD_NOTFOUND); /* command is not recognized as internal */

  /* printf("recognized internal command: '%s', tail of command at offset %u\r\n", cmdptr->cmd, argoffset); */

  /* apply redirections (if any) */
  if (redir_apply(redir) != 0) return(CMD_FAIL);

  /* prepare function parameters and feed it to the cmd handling function */
  p->argc = cmd_explode(p->argvbuf, cmdline + argoffset, p->argv);
  p->env_seg = env_seg;
  p->rmod = rmod;
  p->argoffset = argoffset;
  p->cmdline = cmdline;

  cmdres = (cmdptr->func_ptr)(p);

  /* cancel redirections */
  redir_revert();

  return(cmdres);
}
