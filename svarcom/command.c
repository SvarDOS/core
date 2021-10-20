/*
 * SvarCOM is a command-line interpreter.
 *
 * a little memory area is allocated as high as possible. it contains:
 *  - a signature (like XMS drivers do)
 *  - a routine for exec'ing programs
 *  - a "last command" buffer for input history
 *
 * when svarcom starts, it tries locating the routine in memory.
 *
 * if found:
 *   waits for user input and processes it. if execing something is required, set the "next exec" field in routine's memory and quit.
 *
 * if not found:
 *   installs it by creating a new PSP, set int 22 vector to the routine, set my "parent PSP" to the routine
 *   and quit.
 *
 *
 *
 * good lecture about PSP and allocating memory
 * https://retrocomputing.stackexchange.com/questions/20001/how-much-of-the-program-segment-prefix-area-can-be-reused-by-programs-with-impun/20006#20006
 *
 * PSP structure
 * http://www.piclist.com/techref/dos/psps.htm
 *
 *
 *
 * === MCB ===
 *
 * each time that DOS allocates memory, it prefixes the allocated memory with
 * a 16-bytes structure called a "Memory Control Block" (MCB). This control
 * block has the following structure:
 *
 * Offset  Size     Description
 *   00h   byte     'M' =  non-last member of the MCB chain
 *                  'Z' = indicates last entry in MCB chain
 *                  other values cause "Memory Allocation Failure" on exit
 *   01h   word     PSP segment address of the owner (Process Id)
 *                  possible values:
 *                    0 = free
 *                    8 = Allocated by DOS before first user pgm loaded
 *                    other = Process Id/PSP segment address of owner
 *   03h  word      number of paragraphs related to this MCB (excluding MCB)
 *   05h  11 bytes  reserved
 *   10h  ...       start of actual allocated memory block
 */

#include <i86.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <process.h>

#include "cmd.h"
#include "helpers.h"
#include "rmodinit.h"

struct config {
  int locate;
  int install;
  int envsiz;
} cfg;


static void parse_argv(struct config *cfg, int argc, char **argv) {
  int i;
  memset(cfg, 0, sizeof(*cfg));

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "/locate") == 0) {
      cfg->locate = 1;
    }
    if (strstartswith(argv[i], "/e:") == 0) {
      cfg->envsiz = atoi(argv[i]+3);
      if (cfg->envsiz < 64) cfg->envsiz = 0;
    }
  }
}


static int explode_progparams(char *s, char const **argvlist) {
  int si = 0, argc = 0;
  for (;;) {
    /* skip to next non-space character */
    while (s[si] == ' ') si++;
    /* end of string? */
    if (s[si] == '\r') break;
    /* set argv ptr */
    argvlist[argc++] = s + si;
    /* find next space */
    while (s[si] != ' ' && s[si] != '\r') si++;
    /* is this end of string? */
    if (s[si] == '\r') {
      s[si] = 0;
      break;
    }
    /* not end: terminate arg and look for next one */
    s[si++] = 0;
  }
  /* terminate argvlist with a NULL value */
  argvlist[argc] = NULL;
  return(argc);
}


static void buildprompt(char *s, const char *fmt) {
  for (; *fmt != 0; fmt++) {
    if (*fmt != '$') {
      *s = *fmt;
      s++;
      continue;
    }
    /* escape code ($P, etc) */
    fmt++;
    switch (*fmt) {
      case 'Q':  /* $Q = = (equal sign) */
      case 'q':
        *s = '=';
        s++;
        break;
      case '$':  /* $$ = $ (dollar sign) */
        *s = '$';
        s++;
        break;
      case 'T':  /* $t = current time */
      case 't':
        s += sprintf(s, "00:00"); /* TODO */
        break;
      case 'D':  /* $D = current date */
      case 'd':
        s += sprintf(s, "1985-07-29"); /* TODO */
        break;
      case 'P':  /* $P = current drive and path */
      case 'p':
        _asm {
          mov ah, 0x19    /* DOS 1+ - GET CURRENT DRIVE */
          int 0x21
          mov bx, s
          mov [bx], al  /* AL = drive (00 = A:, 01 = B:, etc */
        }
        *s += 'A';
        s++;
        *s = ':';
        s++;
        *s = '\\';
        s++;
        _asm {
          mov ah, 0x47    /* DOS 2+ - CWD - GET CURRENT DIRECTORY */
          xor dl,dl       /* DL = drive number (00h = default, 01h = A:, etc) */
          mov si, s       /* DS:SI -> 64-byte buffer for ASCIZ pathname */
          int 0x21
        }
        while (*s != 0) s++; /* move ptr forward to end of pathname */
        break;
      case 'V':  /* $V = DOS version number */
      case 'v':
        s += sprintf(s, "VER"); /* TODO */
        break;
      case 'N':  /* $N = current drive */
      case 'n':
        _asm {
          mov ah, 0x19    /* DOS 1+ - GET CURRENT DRIVE */
          int 0x21
          mov bx, s
          mov [bx], al  /* AL = drive (00 = A:, 01 = B:, etc */
        }
        *s += 'A';
        s++;
        break;
      case 'G':  /* $G = > (greater-than sign) */
      case 'g':
        *s = '>';
        s++;
        break;
      case 'L':  /* $L = < (less-than sign) */
      case 'l':
        *s = '<';
        s++;
        break;
      case 'B':  /* $B = | (pipe) */
      case 'b':
        *s = '|';
        s++;
        break;
      case 'H':  /* $H = backspace (erases previous character) */
      case 'h':
        *s = '\b';
        s++;
        break;
      case 'E':  /* $E = Escape code (ASCII 27) */
      case 'e':
        *s = 27;
        s++;
        break;
      case '_':  /* $_ = CR+LF */
        *s = '\r';
        s++;
        *s = '\n';
        s++;
        break;
    }
  }
  *s = '$';
}


int main(int argc, char **argv) {
  struct config cfg;
  unsigned short rmod_seg;
  unsigned short far *rmod_envseg;
  unsigned short far *lastexitcode;

  parse_argv(&cfg, argc, argv);

  rmod_seg = rmod_find();
  if (rmod_seg == 0xffff) {
    rmod_seg = rmod_install(cfg.envsiz);
    if (rmod_seg == 0xffff) {
      puts("ERROR: rmod_install() failed");
      return(1);
    } else {
      printf("rmod installed at seg 0x%04X\r\n", rmod_seg);
    }
  } else {
    printf("rmod found at seg 0x%04x\r\n", rmod_seg);
  }

  rmod_envseg = MK_FP(rmod_seg, RMOD_OFFSET_ENVSEG);
  lastexitcode = MK_FP(rmod_seg, RMOD_OFFSET_LEXITCODE);

  {
    unsigned short envsiz;
    unsigned short far *sizptr = MK_FP(*rmod_envseg - 1, 3);
    envsiz = *sizptr;
    envsiz *= 16;
    printf("rmod_inpbuff at %04X:%04X, env_seg at %04X:0000 (env_size = %u bytes)\r\n", rmod_seg, RMOD_OFFSET_INPBUFF, *rmod_envseg, envsiz);
  }

  for (;;) {
    int i, n, argcount;
    char far *cmdline = MK_FP(rmod_seg, RMOD_OFFSET_INPBUFF + 2);
    char buff[256];
    char const *argvlist[256];

    {
      /* print shell prompt */
      char *promptptr = buff;
      buildprompt(promptptr, "$p$g"); /* TODO: prompt should be configurable via environment */
      _asm {
        push dx
        mov ah, 0x09
        mov dx, promptptr
        int 0x21
        pop dx
      }
    }

    /* wait for user input */
    _asm {
      push ds

      /* is DOSKEY support present? (INT 2Fh, AX=4800h, returns non-zero in AL if present) */
      mov ax, 0x4800
      int 0x2f
      mov bl, al /* save doskey status in BL */

      /* set up buffered input */
      mov ax, rmod_seg
      push ax
      pop ds
      mov dx, RMOD_OFFSET_INPBUFF

      /* execute either DOS input or DOSKEY */
      test bl, bl /* zf set if no DOSKEY present */
      jnz DOSKEY

      mov ah, 0x0a
      int 0x21
      jmp short DONE

      DOSKEY:
      mov ax, 0x4810
      int 0x2f

      DONE:
      pop ds
    }
    printf("\r\n");

    /* if nothing entered, loop again */
    if (cmdline[-1] == 0) continue;

    /* copy buffer to a near var (incl. trailing CR), insert a space before
       every slash to make sure arguments are well separated */
    n = 0;
    for (i = 0; i < cmdline[-1] + 1; i++) {
      if (cmdline[i] == '/') buff[n++] = ' ';
      buff[n++] = cmdline[i];
    }
    buff[n] = 0;

    argcount = explode_progparams(buff, argvlist);

    /* if no args found (eg. all-spaces), loop again */
    if (argcount == 0) continue;

    printf("got %u bytes of cmdline (%d args)\r\n", cmdline[-1], argcount);
    for (i = 0; i < argcount; i++) {
      printf("arg #%d = '%s'\r\n", i, argvlist[i]);
    }

    /* is it about quitting? */
    if (imatch(argvlist[0], "exit")) break;

    /* try running it as an internal command */
    {
      int ecode = cmd_process(argcount, argvlist, *rmod_envseg, cmdline);
      if (ecode >= 0) *lastexitcode = ecode;
      if (ecode >= -1) continue; /* command is internal but did not set an exit code */
    }

    /* must be an external command then */
    execvp(argvlist[0], argvlist);

    /* execvp() replaces the current process by the new one
    if I am still alive then external command failed to execute */
    puts("Bad command or file name");

  }

  return(0);
}
