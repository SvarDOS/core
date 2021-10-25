/* entry point for internal commands
 * matches internal commands and executes them
 * returns -1 or exit code if processed
 * returns -2 if command unrecognized */

#include <i86.h>
#include <stdio.h>
#include <stdlib.h>

#include "doserr.h"
#include "helpers.h"

struct cmd_funcparam {
  int argc;
  const char *argv[256];
  unsigned short env_seg;
  const char far *cmdline;
};

#include "cmd/cd.c"
#include "cmd/dir.c"
#include "cmd/exit.c"
#include "cmd/set.c"

#include "cmd.h"


struct CMD_ID {
  const char *cmd;
  int (*func_ptr)(const struct cmd_funcparam *); /* pointer to handling function */
};

const struct CMD_ID INTERNAL_CMDS[] = {
  {"CD",      cmd_cd},
  {"CHDIR",   cmd_cd},
  {"DIR",     cmd_dir},
  {"EXIT",    cmd_exit},
  {"SET",     cmd_set},
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
    /* find next space while copying arg to local buffer */
    do {
      buff[i++] = s[si++];
    } while (s[si] != ' ' && s[si] != 0 && s[si] != '/');
    buff[i++] = 0;
    /* is this end of string? */
    if (s[si] == 0) break;
  }
  argvlist[argc] = NULL;
  return(argc);
}


int cmd_process(unsigned short env_seg, const char far *cmdline) {
  const struct CMD_ID *cmdptr;
  struct cmd_funcparam p;
  unsigned short argoffset;
  char cmdbuff[256];

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
  p.argc = cmd_explode(cmdbuff, cmdline + argoffset, p.argv);
  p.env_seg = env_seg;
  p.cmdline = cmdline;

  return((cmdptr->func_ptr)(&p));
}
