/* This file is part of the SvarCOM project and is published under the terms
 * of the MIT license.
 *
 * Copyright (C) 2021-2024 Mateusz Viste
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

/* entry point for internal commands, it matches internal commands and
 * executes them.
 *
 * returns one of the following values:
 *   CMD_OK               command executed successfully
 *   CMD_FAIL             command ended in error
 *   CMD_CHANGED          command-line has been modified (used by IF)
 *   CMD_CHANGED_BY_CALL  command-line has been modified by CALL
 *   CMD_CHANGED_BY_LH    command-line has been modified by LOADHIGH
 *   CMD_NOTFOUND         command unrecognized
 */

#include <i86.h>

#include "svarlang.lib/svarlang.h"

#include "env.h"
#include "helpers.h"
#include "redir.h"
#include "rmodinit.h"
#include "sayonara.h"
#include "version.h"

#include "cmd.h"


/* struct used to pass all necessary information to the sub-commands */
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


/* scans argv for the presence of a "/?" parameter.
 * returns 1 if found, 0 otherwise
 * this is used by most sub-commands to detect /? invocations */
static int cmd_ishlp(const struct cmd_funcparam *p) {
  int i;
  for (i = 0; i < p->argc; i++) {
    if ((p->argv[i][0] == '/') && (p->argv[i][1] == '?')) return(1);
  }
  return(0);
}

#include "cmd/break.c"
#include "cmd/call.c"
#include "cmd/cd.c"
#include "cmd/chcp.c"
#include "cmd/cls.c"
#include "cmd/copy.c"
#include "cmd/ctty.c"
#include "cmd/date.c"
#include "cmd/del.c"
#include "cmd/for.c"
#include "cmd/goto.c"
#include "cmd/if.c"
#include "cmd/vol.c"     /* must be included before dir.c due to dependency */
#include "cmd/dir.c"
#include "cmd/echo.c"
#include "cmd/exit.c"
#include "cmd/loadhigh.c"
#include "cmd/ln.c"
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
#include "cmd/truename.c"
#include "cmd/type.c"
#include "cmd/ver.c"
#include "cmd/verify.c"


struct CMD_ID {
  const char *cmd;
  enum cmd_result (*func_ptr)(struct cmd_funcparam *); /* pointer to handling function */
};

const struct CMD_ID INTERNAL_CMDS[] = {
  {"BREAK",   cmd_break},
  {"CALL",    cmd_call},
  {"CD",      cmd_cd},
  {"CHCP",    cmd_chcp},
  {"CHDIR",   cmd_cd},
  {"CLS",     cmd_cls},
  {"COPY",    cmd_copy},
  {"CTTY",    cmd_ctty},
  {"DATE",    cmd_date},
  {"DEL",     cmd_del},
  {"DIR",     cmd_dir},
  {"ECHO",    cmd_echo},
  {"ERASE",   cmd_del},
  {"EXIT",    cmd_exit},
  {"FOR",     cmd_for},
  {"GOTO",    cmd_goto},
  {"IF",      cmd_if},
  {"LH",      cmd_loadhigh},
  {"LN",      cmd_ln},
  {"LOADHIGH",cmd_loadhigh},
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
  {"TRUENAME",cmd_truename},
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


/* asks DOS to change the current default drive, returns the current drive
 * (should be the same as d on success) */
static unsigned char _changedosdrive(unsigned char d);
#pragma aux _changedosdrive = \
"mov ah, 0x0e"   /* DOS 1+ - SELECT DRIVE (DL=drive, 00h=A:, 01h=B:, etc) */ \
"int 0x21" \
"mov ah, 0x19"     /* DOS 1+ - GET CURRENT DRIVE (drive in al) */ \
"int 0x21" \
parm [dl] \
modify [ah] \
value [al]


/* asks DOS to del a file, returns 0 on success */
static unsigned short _dosdelfile(const char near *f);
#pragma aux _dosdelfile = \
"mov ah, 0x41"  /* delete a file ; DS:DX = filename to delete */ \
"int 0x21" \
"jc DONE" \
"xor ax, ax" \
"DONE:" \
parm [dx] \
value [ax]


enum cmd_result cmd_process(struct rmod_props far *rmod, unsigned short env_seg, const char *cmdline, void *BUFFERPTR, unsigned short BUFFERSZ, const struct redir_data *redir, unsigned char delstdin) {
  const struct CMD_ID *cmdptr;
  unsigned short argoffset;
  enum cmd_result cmdres;
  struct cmd_funcparam *p = (void *)BUFFERPTR;
  p->BUFFERSZ = BUFFERSZ - sizeof(*p);

  /* special case: is this a drive change? (like "E:") */
  if ((cmdline[0] != 0) && (cmdline[1] == ':') && ((cmdline[2] == ' ') || (cmdline[2] == 0))) {
    if (((cmdline[0] >= 'a') && (cmdline[0] <= 'z')) || ((cmdline[0] >= 'A') && (cmdline[0] <= 'Z'))) {
      unsigned char drive = cmdline[0];
      unsigned char curdrive;
      if (drive >= 'a') {
        drive -= 'a';
      } else {
        drive -= 'A';
      }
      curdrive = _changedosdrive(drive);
      if (curdrive == drive) return(CMD_OK);
      nls_outputnl_doserr(0x0f);
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

  /* delete stdin temporary file */
  if (delstdin) {
    unsigned short doserr = _dosdelfile(redir->stdinfile);
    if (doserr) {
      output(redir->stdinfile);
      output(": ");
      nls_outputnl_doserr(doserr);
    }
  }

  return(cmdres);
}
