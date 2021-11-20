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
#include "env.h"
#include "helpers.h"
#include "redir.h"
#include "rmodinit.h"
#include "sayonara.h"

#include "rmodcore.h" /* rmod binary inside a BUFFER array */

struct config {
  unsigned char flags; /* command.com flags, as defined in rmodinit.h */
  char *execcmd;
  unsigned short envsiz;
};

/* max length of the cmdline storage (bytes) - includes also max length of
 * line loaded from a BAT file (no more than 255 bytes!) */
#define CMDLINE_MAXLEN 255


/* sets guard values at a few places in memory for later detection of
 * overflows via memguard_check() */
static void memguard_set(void) {
  BUFFER[sizeof(BUFFER) - 1] = 0xC7;
  BUFFER[sizeof(BUFFER) - (CMDLINE_MAXLEN + 3)] = 0xC7;
}


/* checks for valguards at specific memory locations, returns 0 on success */
static int memguard_check(unsigned short rmodseg) {
  /* check RMOD signature (would be overwritten in case of stack overflow */
  static char msg[] = "!! MEMORY CORRUPTION ## DETECTED !!";
  unsigned short far *rmodsig = MK_FP(rmodseg, 0x100 + 6);
  if (*rmodsig != 0x2019) {
    msg[22] = '1';
    outputnl(msg);
    printf("rmodseg = %04X ; *rmodsig = %04X\r\n", rmodseg, *rmodsig);
    return(1);
  }
  /* check last BUFFER byte (could be overwritten by cmdline) */
  if (BUFFER[sizeof(BUFFER) - 1] != 0xC7) {
    msg[22] = '2';
    outputnl(msg);
    return(2);
  }
  /* check that cmdline BUFFER's end hasn't been touched by something else */
  if (BUFFER[sizeof(BUFFER) - (CMDLINE_MAXLEN + 3)] != 0xC7) {
    msg[22] = '3';
    outputnl(msg);
    return(3);
  }
  /* all good */
  return(0);
}


/* parses command line the hard way (directly from PSP) */
static void parse_argv(struct config *cfg) {
  unsigned short i;
  const unsigned char *cmdlinelen = (unsigned char *)0x80;
  char *cmdline = (char *)0x81;

  memset(cfg, 0, sizeof(*cfg));

  /* set a NULL terminator on cmdline */
  cmdline[*cmdlinelen] = 0;

  for (i = 0;;) {

    /* skip over any leading spaces */
    for (;; i++) {
      if (cmdline[i] == 0) return;
      if (cmdline[i] != ' ') break;
    }

    if (cmdline[i] != '/') {
      output("Invalid parameter: ");
      outputnl(cmdline + i);
      /* exit(1); */
    } else {
      i++;        /* skip the slash */
      switch (cmdline[i]) {
        case 'c': /* /C = execute command and quit */
        case 'C':
          cfg->flags |= FLAG_EXEC_AND_QUIT;
          /* FALLTHRU */
        case 'k': /* /K = execute command and keep running */
        case 'K':
          cfg->execcmd = cmdline + i + 1;
          return;

        case 'e': /* preset the initial size of the environment block */
        case 'E':
          i++;
          if (cmdline[i] == ':') i++; /* could be /E:size */
          atous(&(cfg->envsiz), cmdline + i);
          if (cfg->envsiz < 64) cfg->envsiz = 0;
          break;

        case 'p': /* permanent shell (can't exit + run autoexec.bat) */
        case 'P':
          cfg->flags |= FLAG_PERMANENT;
          break;

        case '?':
          outputnl("Starts the SvarCOM command interpreter");
          outputnl("");
          outputnl("COMMAND /E:nnn [/[C|K] command]");
          outputnl("");
          outputnl("/E:nnn     Sets the environment size to nnn bytes");
          outputnl("/P         Makes the new command interpreter permanent (can't exit)");
          outputnl("/C         Executes the specified command and returns");
          outputnl("/K         Executes the specified command and continues running");
          exit(1);
          break;

        default:
          output("Invalid switch:");
          output(" ");
          outputnl(cmdline + i);
          exit(1);
          break;
      }
    }

    /* move to next argument or quit processing if end of cmdline */
    for (i++; (cmdline[i] != 0) && (cmdline[i] != ' ') && (cmdline[i] != '/'); i++);

  }
}


/* builds the prompt string and displays it. buff is filled with a zero-terminated copy of the prompt. */
static void build_and_display_prompt(char *buff, unsigned short envseg) {
  char *s = buff;
  /* locate the prompt variable or use the default pattern */
  const char far *fmt = env_lookup_val(envseg, "PROMPT");
  if ((fmt == NULL) || (*fmt == 0)) fmt = "$p$g"; /* fallback to default if empty */
  /* build the prompt string based on pattern */
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
  *s = 0;
  output(buff);
}


/* tries locating executable fname in path and fille res with result. returns 0 on success,
 * -1 on failed match and -2 on failed match + "don't even try with other paths"
 * format is filled the offset where extension starts in fname (-1 if not found) */
int lookup_cmd(char *res, const char *fname, const char *path, const char **extptr) {
  unsigned short lastbslash = 0xffff;
  unsigned short i, len;
  unsigned char explicitpath = 0;

  /* does the original fname had an explicit path prefix or explicit ext? */
  *extptr = NULL;
  for (i = 0; fname[i] != 0; i++) {
    switch (fname[i]) {
      case ':':
      case '\\':
        explicitpath = 1;
        *extptr = NULL; /* extension is the last dot AFTER all path delimiters */
        break;
      case '.':
        *extptr = fname + i + 1;
        break;
    }
  }

  /* normalize filename */
  if (file_truename(fname, res) != 0) return(-2);

  /* printf("truename: %s\r\n", res); */

  /* figure out where the command starts and if it has an explicit extension */
  for (len = 0; res[len] != 0; len++) {
    switch (res[len]) {
      case '?':   /* abort on any wildcard character */
      case '*':
        return(-2);
      case '\\':
        lastbslash = len;
        break;
    }
  }

  /* printf("lastbslash=%u\r\n", lastbslash); */

  /* if no path prefix in fname (':' or backslash), then assemble path+filename */
  if (!explicitpath) {
    if (path != NULL) {
      i = strlen(path);
    } else {
      i = 0;
    }
    if ((i != 0) && (path[i - 1] != '\\')) i++; /* add a byte for inserting a bkslash after path */
    memmove(res + i, res + lastbslash + 1, len - lastbslash);
    if (i != 0) {
      memmove(res, path, i);
      res[i - 1] = '\\';
    }
  }

  /* if no extension was initially provided, try matching COM, EXE, BAT */
  if (*extptr == NULL) {
    const char *ext[] = {".COM", ".EXE", ".BAT", NULL};
    len = strlen(res);
    for (i = 0; ext[i] != NULL; i++) {
      strcpy(res + len, ext[i]);
      /* printf("? '%s'\r\n", res); */
      *extptr = ext[i] + 1;
      if (file_getattr(res) >= 0) return(0);
    }
  } else { /* try finding it as-is */
    /* printf("? '%s'\r\n", res); */
    if (file_getattr(res) >= 0) return(0);
  }

  /* not found */
  if (explicitpath) return(-2); /* don't bother trying other paths, the caller had its own path preset anyway */
  return(-1);
}


static void run_as_external(char *buff, const char *cmdline, unsigned short envseg, struct rmod_props far *rmod) {
  char *cmdfile = buff + 512;
  const char far *pathptr;
  int lookup;
  unsigned short i;
  const char *ext;
  char *cmd = buff + 256;
  const char *cmdtail;
  char far *rmod_execprog = MK_FP(rmod->rmodseg, RMOD_OFFSET_EXECPROG);
  char far *rmod_cmdtail = MK_FP(rmod->rmodseg, 0x81);
  _Packed struct {
    unsigned short envseg;
    unsigned long cmdtail;
    unsigned long fcb1;
    unsigned long fcb2;
  } far *ExecParam = MK_FP(rmod->rmodseg, RMOD_OFFSET_EXECPARAM);

  /* find cmd and cmdtail */
  i = 0;
  cmdtail = cmdline;
  while (*cmdtail == ' ') cmdtail++; /* skip any leading spaces */
  while ((*cmdtail != ' ') && (*cmdtail != '/') && (*cmdtail != '+') && (*cmdtail != 0)) {
    cmd[i++] = *cmdtail;
    cmdtail++;
  }
  cmd[i] = 0;

  /* is this a command in curdir? */
  lookup = lookup_cmd(cmdfile, cmd, NULL, &ext);
  if (lookup == 0) {
    /* printf("FOUND LOCAL EXEC FILE: '%s'\r\n", cmdfile); */
    goto RUNCMDFILE;
  } else if (lookup == -2) {
    /* puts("NOT FOUND"); */
    return;
  }

  /* try matching something in PATH */
  pathptr = env_lookup_val(envseg, "PATH");
  if (pathptr == NULL) return;

  /* try each path in %PATH% */
  for (;;) {
    for (i = 0;; i++) {
      buff[i] = *pathptr;
      if ((buff[i] == 0) || (buff[i] == ';')) break;
      pathptr++;
    }
    buff[i] = 0;
    lookup = lookup_cmd(cmdfile, cmd, buff, &ext);
    if (lookup == 0) break;
    if (lookup == -2) return;
    if (*pathptr == ';') {
      pathptr++;
    } else {
      return;
    }
  }

  RUNCMDFILE:

  /* special handling of batch files */
  if ((ext != NULL) && (imatch(ext, "bat"))) {
    /* copy truename of the bat file to rmod buff */
    for (i = 0; cmdfile[i] != 0; i++) rmod->batfile[i] = cmdfile[i];
    rmod->batfile[i] = 0;
    /* copy args of the bat file to rmod buff */
    for (i = 0; cmdtail[i] != 0; i++) rmod->batargs[i] = cmdtail[i];
    /* reset the 'next line to execute' counter */
    rmod->batnextline = 0;
    /* remember the echo flag (in case bat file disables echo) */
    rmod->flags &= ~FLAG_ECHO_BEFORE_BAT;
    if (rmod->flags & FLAG_ECHOFLAG) rmod->flags |= FLAG_ECHO_BEFORE_BAT;
    return;
  }

  /* copy full filename to execute */
  for (i = 0; cmdfile[i] != 0; i++) rmod_execprog[i] = cmdfile[i];
  rmod_execprog[i] = 0;

  /* copy cmdtail to rmod's PSP and compute its len */
  for (i = 0; cmdtail[i] != 0; i++) rmod_cmdtail[i] = cmdtail[i];
  rmod_cmdtail[i] = '\r';
  rmod_cmdtail[-1] = i;

  /* set up rmod to execute the command */

  ExecParam->envseg = envseg;
  ExecParam->cmdtail = (unsigned long)MK_FP(rmod->rmodseg, 0x80); /* farptr, must be in PSP format (lenbyte args \r) */
  ExecParam->fcb1 = 0; /* TODO farptr */
  ExecParam->fcb2 = 0; /* TODO farptr */
  exit(0); /* let rmod do the job now */
}


static void set_comspec_to_self(unsigned short envseg) {
  unsigned short *psp_envseg = (void *)(0x2c); /* pointer to my env segment field in the PSP */
  char far *myenv = MK_FP(*psp_envseg, 0);
  unsigned short varcount;
  char buff[256] = "COMSPEC=";
  char *buffptr = buff + 8;
  /* who am i? look into my own environment, at the end of it should be my EXEPATH string */
  while (*myenv != 0) {
    /* consume a NULL-terminated string */
    while (*myenv != 0) myenv++;
    /* move to next string */
    myenv++;
  }
  /* get next word, if 1 then EXEPATH follows */
  myenv++;
  varcount = *myenv;
  myenv++;
  varcount |= (*myenv << 8);
  myenv++;
  if (varcount != 1) return; /* NO EXEPATH FOUND */
  while (*myenv != 0) {
    *buffptr = *myenv;
    buffptr++;
    myenv++;
  }
  *buffptr = 0;
  /* printf("EXEPATH: '%s'\r\n", buff); */
  env_setvar(envseg, buff);
}


/* wait for user input */
static void cmdline_getinput(unsigned short inpseg, unsigned short inpoff) {
  _asm {
    push ax
    push bx
    push cx
    push dx
    push ds

    /* is DOSKEY support present? (INT 2Fh, AX=4800h, returns non-zero in AL if present) */
    mov ax, 0x4800
    int 0x2f
    mov bl, al /* save doskey status in BL */

    /* set up buffered input to inpseg:inpoff */
    mov ax, inpseg
    push ax
    pop ds
    mov dx, inpoff

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
    /* terminate command with a CR/LF */
    mov ah, 0x02 /* display character in dl */
    mov dl, 0x0d
    int 0x21
    mov dl, 0x0a
    int 0x21

    pop ds
    pop dx
    pop cx
    pop bx
    pop ax
  }
}


/* fetches a line from batch file and write it to buff (NULL-terminated),
 * increments rmod counter and returns 0 on success. */
static int getbatcmd(char *buff, unsigned char buffmaxlen, struct rmod_props far *rmod) {
  unsigned short i;
  unsigned short batname_seg = FP_SEG(rmod->batfile);
  unsigned short batname_off = FP_OFF(rmod->batfile);
  unsigned short filepos_cx = rmod->batnextline >> 16;
  unsigned short filepos_dx = rmod->batnextline & 0xffff;
  unsigned char blen = 0;

  /* open file, jump to offset filpos, and read data into buff.
   * result in blen (unchanged if EOF or failure). */
  _asm {
    push ax
    push bx
    push cx
    push dx

    /* open file (read-only) */
    mov dx, batname_off
    mov ax, batname_seg
    push ds     /* save DS */
    mov ds, ax
    mov ax, 0x3d00
    int 0x21    /* handle in ax on success */
    pop ds      /* restore DS */
    jc DONE
    mov bx, ax  /* save handle to bx */

    /* jump to file offset CX:DX */
    mov ax, 0x4200
    mov cx, filepos_cx
    mov dx, filepos_dx
    int 0x21  /* CF clear on success, DX:AX set to cur pos */
    jc CLOSEANDQUIT

    /* read the line into buff */
    mov ah, 0x3f
    xor ch, ch
    mov cl, buffmaxlen
    mov dx, buff
    int 0x21 /* CF clear on success, AX=number of bytes read */
    jc CLOSEANDQUIT
    mov blen, al

    CLOSEANDQUIT:
    /* close file (handle in bx) */
    mov ah, 0x3e
    int 0x21

    DONE:
    pop dx
    pop cx
    pop bx
    pop ax
  }

  /* printf("blen=%u filepos_cx=%u filepos_dx=%u\r\n", blen, filepos_cx, filepos_dx); */

  /* on EOF - abort processing the bat file */
  if (blen == 0) goto OOPS;

  /* find nearest \n to inc batch offset and replace \r by NULL terminator
   * I support all CR/LF, CR- and LF-terminated batch files */
  for (i = 0; i < blen; i++) {
    if ((buff[i] == '\r') || (buff[i] == '\n')) {
      if ((buff[i] == '\r') && ((i+1) < blen) && (buff[i+1] == '\n')) rmod->batnextline += 1;
      break;
    }
  }
  buff[i] = 0;
  rmod->batnextline += i + 1;

  return(0);

  OOPS:
  rmod->batfile[0] = 0;
  rmod->batnextline = 0;
  return(-1);
}


int main(void) {
  static struct config cfg;
  static unsigned short far *rmod_envseg;
  static unsigned short far *lastexitcode;
  static struct rmod_props far *rmod;
  static char *cmdline;

  rmod = rmod_find(BUFFER_len);
  if (rmod == NULL) {
    /* look at command line parameters (in case env size if set there) */
    parse_argv(&cfg);
    rmod = rmod_install(cfg.envsiz, BUFFER, BUFFER_len);
    if (rmod == NULL) {
      outputnl("ERROR: rmod_install() failed");
      return(1);
    }
    /* copy flags to rmod's storage (and enable ECHO) */
    rmod->flags = cfg.flags | FLAG_ECHOFLAG;
    /* printf("rmod installed at %Fp\r\n", rmod); */
  } else {
    /* printf("rmod found at %Fp\r\n", rmod); */
    /* if I was spawned by rmod and FLAG_EXEC_AND_QUIT is set, then I should
     * die asap, because the command has been executed already, so I no longer
     * have a purpose in life */
    if (rmod->flags & FLAG_EXEC_AND_QUIT) sayonara(rmod);
  }

  /* install a few guardvals in memory to detect some cases of overflows */
  memguard_set();

  rmod_envseg = MK_FP(rmod->rmodseg, RMOD_OFFSET_ENVSEG);
  lastexitcode = MK_FP(rmod->rmodseg, RMOD_OFFSET_LEXITCODE);

  /* make COMPSEC point to myself */
  set_comspec_to_self(*rmod_envseg);

/*  {
    unsigned short envsiz;
    unsigned short far *sizptr = MK_FP(*rmod_envseg - 1, 3);
    envsiz = *sizptr;
    envsiz *= 16;
    printf("rmod_inpbuff at %04X:%04X, env_seg at %04X:0000 (env_size = %u bytes)\r\n", rmod->rmodseg, RMOD_OFFSET_INPBUFF, *rmod_envseg, envsiz);
  }*/

  /* on /P check for the presence of AUTOEXEC.BAT and execute it if found */
  if (cfg.flags & FLAG_PERMANENT) {
    if (file_getattr("AUTOEXEC.BAT") >= 0) cfg.execcmd = "AUTOEXEC.BAT";
  }

  do {
    /* terminate previous command with a CR/LF if ECHO ON (but not during BAT processing) */
    if ((rmod->flags & FLAG_ECHOFLAG) && (rmod->batfile[0] == 0)) outputnl("");

    SKIP_NEWLINE:

    /* cancel any redirections that may have been set up before */
    redir_revert();

    /* memory check */
    memguard_check(rmod->rmodseg);

    /* preset cmdline to point at the end of my general-purpose buffer (with
     * one extra byte for the NULL terminator and another for memguard val) */
    cmdline = BUFFER + sizeof(BUFFER) - (CMDLINE_MAXLEN + 2);

    /* (re)load translation strings if needed */
    nls_langreload(BUFFER, *rmod_envseg);

    /* skip user input if I have a command to exec (/C or /K) */
    if (cfg.execcmd != NULL) {
      cmdline = cfg.execcmd;
      cfg.execcmd = NULL;
      goto EXEC_CMDLINE;
    }

    /* if batch file is being executed -> fetch next line */
    if (rmod->batfile[0] != 0) {
      if (getbatcmd(cmdline, CMDLINE_MAXLEN, rmod) != 0) { /* end of batch */
        /* restore echo flag as it was before running the bat file */
        rmod->flags &= ~FLAG_ECHOFLAG;
        if (rmod->flags & FLAG_ECHO_BEFORE_BAT) rmod->flags |= FLAG_ECHOFLAG;
        continue;
      }
      /* skip any leading spaces */
      while (*cmdline == ' ') cmdline++;
      /* output prompt and command on screen if echo on and command is not
       * inhibiting it with the @ prefix */
      if ((rmod->flags & FLAG_ECHOFLAG) && (cmdline[0] != '@')) {
        build_and_display_prompt(BUFFER, *rmod_envseg);
        outputnl(cmdline);
      }
      /* skip the @ prefix if present, it is no longer useful */
      if (cmdline[0] == '@') cmdline++;
    } else {
      /* interactive mode: display prompt (if echo enabled) and wait for user
       * command line */
      if (rmod->flags & FLAG_ECHOFLAG) build_and_display_prompt(BUFFER, *rmod_envseg);
      /* collect user input */
      cmdline_getinput(FP_SEG(rmod->inputbuf), FP_OFF(rmod->inputbuf));
      /* copy it to local cmdline */
      if (rmod->inputbuf[1] != 0) _fmemcpy(cmdline, rmod->inputbuf + 2, rmod->inputbuf[1]);
      cmdline[(unsigned)(rmod->inputbuf[1])] = 0; /* zero-terminate local buff (oriignal is '\r'-terminated) */
    }

    /* if nothing entered, loop again (but without appending an extra CR/LF) */
    if (cmdline[0] == 0) goto SKIP_NEWLINE;

    /* I jump here when I need to exec an initial command (/C or /K) */
    EXEC_CMDLINE:

    /* move pointer forward to skip over any leading spaces */
    while (*cmdline == ' ') cmdline++;

    /* update rmod's ptr to COMPSPEC so it is always up to date */
    rmod_updatecomspecptr(rmod->rmodseg, *rmod_envseg);

    /* handle redirections (if any) */
    if (redir_parsecmd(cmdline, BUFFER) != 0) {
      outputnl("");
      continue;
    }

    /* try matching (and executing) an internal command */
    if (cmd_process(rmod, *rmod_envseg, cmdline, BUFFER) >= -1) {
      /* internal command executed */
      redir_revert(); /* revert stdout (in case it was redirected) */
      continue;
    }

    /* if here, then this was not an internal command */
    run_as_external(BUFFER, cmdline, *rmod_envseg, rmod);
    /* perhaps this is a newly launched BAT file */
    if ((rmod->batfile[0] != 0) && (rmod->batnextline == 0)) goto SKIP_NEWLINE;

    /* revert stdout (so the err msg is not redirected) */
    redir_revert();

    /* run_as_external() does not return on success, if I am still alive then
     * external command failed to execute */
    outputnl("Bad command or file name");

  } while ((rmod->flags & FLAG_EXEC_AND_QUIT) == 0);

  sayonara(rmod);
  return(0);
}
