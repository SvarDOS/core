/* entry point for internal commands
 * matches internal commands and executes them
 * returns -1 or exit code if processed
 * returns -2 if command unrecognized */

#include <i86.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "doserr.h"
#include "env.h"
#include "helpers.h"
#include "rmodinit.h"

#define BUFFER_SIZE 2048    /* make sure this is not bigger than the static buffer in command.c */

struct cmd_funcparam {
  int argc;                 /* number of arguments */
  const char *argv[256];    /* pointers to each argument */
  unsigned short env_seg;   /* segment of environment block */
  unsigned short rmod_seg;  /* segment of the resident module */
  unsigned short argoffset; /* offset of cmdline where first argument starts */
  const char far *cmdline;  /* original cmdline (terminated by a NULL) */
  char BUFFER[BUFFER_SIZE]; /* a buffer for whatever is needed */
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
#include "cmd/cd.c"
#include "cmd/chcp.c"
#include "cmd/cls.c"
#include "cmd/copy.c"
#include "cmd/del.c"
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
#include "cmd/type.c"
#include "cmd/ver.c"
#include "cmd/verify.c"

#include "cmd.h"


struct CMD_ID {
  const char *cmd;
  int (*func_ptr)(struct cmd_funcparam *); /* pointer to handling function */
};

const struct CMD_ID INTERNAL_CMDS[] = {
  {"BREAK",   cmd_notimpl},
  {"CD",      cmd_cd},
  {"CHCP",    cmd_chcp},
  {"CHDIR",   cmd_cd},
  {"CLS",     cmd_cls},
  {"COPY",    cmd_copy},
  {"CTTY",    cmd_notimpl},
  {"DATE",    cmd_notimpl},
  {"DEL",     cmd_del},
  {"DIR",     cmd_dir},
  {"ECHO",    cmd_echo},
  {"ERASE",   cmd_del},
  {"EXIT",    cmd_exit},
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
  {"TIME",    cmd_notimpl},
  {"TYPE",    cmd_type},
  {"VER",     cmd_ver},
  {"VERIFY",  cmd_verify},
  {"VOL",     cmd_vol},
  {NULL,      NULL}
};


/* NULL if cmdline is not matching an internal command, otherwise returns a
 * pointer to a CMD_ID struct */
static const struct CMD_ID *cmd_match(const char far *cmdline, unsigned short *argoffset) {
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


/* explodes a command into an array of arguments where last arg is NULL
 * returns number of args */
unsigned short cmd_explode(char *buff, const char far *s, char const **argvlist) {
  int si = 0, argc = 0, i = 0;
  for (;;) {
    /* skip to next non-space character */
    while (s[si] == ' ') si++;
    /* end of string? */
    if (s[si] == 0) break;
    /* set argv ptr */
    argvlist[argc++] = buff + i;
    /* find next arg delimiter (spc, null, slash or plus) while copying arg to local buffer */
    do {
      buff[i++] = s[si++];
    } while (s[si] != ' ' && s[si] != 0 && s[si] != '/' && s[si] != '+');
    buff[i++] = 0;
    /* is this end of string? */
    if (s[si] == 0) break;
  }
  argvlist[argc] = NULL;
  return(argc);
}


int cmd_process(unsigned short rmod_seg, unsigned short env_seg, const char far *cmdline, char *BUFFER) {
  const struct CMD_ID *cmdptr;
  unsigned short argoffset;
  struct cmd_funcparam *p = (void *)BUFFER;

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
      if (curdrive != drive) puts(doserr(0x0f));
      return(-1);
    }
  }

  /* try matching an internal command */
  cmdptr = cmd_match(cmdline, &argoffset);
  if (cmdptr == NULL) return(-2); /* command is not recognized as internal */

  /* printf("recognized internal command: '%s', tail of command at offset %u\r\n", cmdptr->cmd, argoffset); */

  /* prepare function parameters and feed it to the cmd handling function */
  p->argc = cmd_explode(BUFFER + sizeof(*p), cmdline + argoffset, p->argv);
  p->env_seg = env_seg;
  p->rmod_seg = rmod_seg;
  p->argoffset = argoffset;
  p->cmdline = cmdline;

  return((cmdptr->func_ptr)(p));
}
