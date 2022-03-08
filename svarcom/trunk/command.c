/* This file is part of the SvarCOM project and is published under the terms
 * of the MIT license.
 *
 * Copyright (C) 2021-2022 Mateusz Viste
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

#include <i86.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "svarlang.lib/svarlang.h"

#include "cmd.h"
#include "env.h"
#include "helpers.h"
#include "redir.h"
#include "rmodinit.h"
#include "sayonara.h"

#include "rmodcore.h" /* rmod binary inside a BUFFER array */

/* this version byte is used to tag RMOD so I can easily make sure that
 * the RMOD struct I find in memory is one that I know. Should the version
 * mismatch, then it would likely mean that SvarCOM has been upgraded and
 * RMOD should not be accessed as its structure might no longer be in sync
 * with what I think it is.
 *          *** INCREMENT THIS AT EACH NEW SVARCOM RELEASE! ***          */
#define BYTE_VERSION 4


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
static void memguard_set(char *cmdlinebuf) {
  BUFFER[sizeof(BUFFER) - 1] = 0xC7;
  cmdlinebuf[CMDLINE_MAXLEN] = 0xC7;
}


/* checks for valguards at specific memory locations, returns 0 on success */
static int memguard_check(unsigned short rmodseg, char *cmdlinebuf) {
  /* check RMOD signature (would be overwritten in case of stack overflow */
  static char msg[] = "!! MEMORY CORRUPTION ## DETECTED !!";
  unsigned short far *rmodsig = MK_FP(rmodseg, 0x100 + 6);
  unsigned char far *rmod = MK_FP(rmodseg, 0);

  if (*rmodsig != 0x2019) {
    msg[22] = '1';
    goto FAIL;
  }

  /* check last BUFFER byte */
  if (BUFFER[sizeof(BUFFER) - 1] != 0xC7) {
    msg[22] = '2';
    goto FAIL;
  }

  /* check last cmdlinebuf byte */
  if (cmdlinebuf[CMDLINE_MAXLEN] != 0xC7) {
    msg[22] = '3';
    goto FAIL;
  }

  /* check rmod exec buf */
  if (rmod[RMOD_OFFSET_EXECPROG + 127] != 0) {
    msg[22] = '4';
    goto FAIL;
  }

  /* check rmod exec stdin buf */
  if (rmod[RMOD_OFFSET_STDINFILE + 127] != 0) {
    msg[22] = '5';
    goto FAIL;
  }

  /* check rmod exec stdout buf */
  if (rmod[RMOD_OFFSET_STDOUTFILE + 127] != 0) {
    msg[22] = '6';
    goto FAIL;
  }

  /* else all good */
  return(0);

  /* error handling */
  FAIL:
  outputnl(msg);
  return(1);
}


/* parses command line the hard way (directly from PSP) */
static void parse_argv(struct config *cfg) {
  const unsigned char *cmdlinelen = (void *)0x80;
  char *cmdline = (void *)0x81;

  memset(cfg, 0, sizeof(*cfg));

  /* set a NULL terminator on cmdline */
  cmdline[*cmdlinelen] = 0;

  while (*cmdline != 0) {

    /* skip over any leading spaces */
    if (*cmdline == ' ') {
      cmdline++;
      continue;
    }

    if (*cmdline != '/') {
      nls_output(0,6); /* "Invalid parameter" */
      output(": ");
      outputnl(cmdline);
      goto SKIP_TO_NEXT_ARG;
    }

    /* got a slash */
    cmdline++;  /* skip the slash */
    switch (*cmdline) {
      case 'c': /* /C = execute command and quit */
      case 'C':
        cfg->flags |= FLAG_EXEC_AND_QUIT;
        /* FALLTHRU */
      case 'k': /* /K = execute command and keep running */
      case 'K':
        cmdline++;
        cfg->execcmd = cmdline;
        return; /* further arguments are for the executed program, not for me */

      case 'y': /* /Y = execute batch file step-by-step (with /P, /K or /C) */
      case 'Y':
        cfg->flags |= FLAG_STEPBYSTEP;
        break;

      case 'd': /* /D = skip autoexec.bat processing */
      case 'D':
        cfg->flags |= FLAG_SKIP_AUTOEXEC;
        break;

      case 'e': /* preset the initial size of the environment block */
      case 'E':
        cmdline++;
        if (*cmdline == ':') cmdline++; /* could be /E:size */
        atous(&(cfg->envsiz), cmdline);
        if (cfg->envsiz < 64) cfg->envsiz = 0;
        break;

      case 'p': /* permanent shell (can't exit + run autoexec.bat) */
      case 'P':
        cfg->flags |= FLAG_PERMANENT;
        break;

      case '?':
        nls_outputnl(1,0); /* "Starts the SvarCOM command interpreter" */
        outputnl("");
        nls_outputnl(1,1); /* "COMMAND /E:nnn [/[C|K] [/P] [/D] command]" */
        outputnl("");
        nls_outputnl(1,2); /* "/D      Skip AUTOEXEC.BAT processing (makes sense only with /P)" */
        nls_outputnl(1,3); /* "/E:nnn  Sets the environment size to nnn bytes" */
        nls_outputnl(1,4); /* "/P      Makes the new command interpreter permanent and run AUTOEXEC.BAT" */
        nls_outputnl(1,5); /* "/C      Executes the specified command and returns" */
        nls_outputnl(1,6); /* "/K      Executes the specified command and continues running" */
        nls_outputnl(1,7); /* "/Y      Executes the batch program step by step" */
        exit(1);
        break;

      default:
        nls_output(0,2); /* invalid switch */
        output(": /");
        outputnl(cmdline);
        break;
    }

    /* move to next argument or quit processing if end of cmdline */
    SKIP_TO_NEXT_ARG:
    while ((*cmdline != 0) && (*cmdline != ' ') && (*cmdline != '/')) cmdline++;
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
          jc DONE         /* leave path empty on error */
          /* move s ptr forward to end (0-termintor) of pathname */
          NEXTBYTE:
          mov si, s
          cmp byte ptr [si], 0
          je DONE
          inc s
          jmp NEXTBYTE
          DONE:
        }
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


/* a few internal flags */
#define DELETE_STDIN_FILE 1
#define CALL_FLAG         2

static void run_as_external(char *buff, const char *cmdline, unsigned short envseg, struct rmod_props far *rmod, struct redir_data *redir, unsigned char flags) {
  char *cmdfile = buff + 512;
  const char far *pathptr;
  int lookup;
  unsigned short i;
  const char *ext;
  char *cmd = buff + 1024;
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

  /* try each path in %PATH% */
  while (pathptr) {
    for (i = 0;; i++) {
      buff[i] = *pathptr;
      if ((buff[i] == 0) || (buff[i] == ';')) break;
      pathptr++;
    }
    buff[i] = 0;
    lookup = lookup_cmd(cmdfile, cmd, buff, &ext);
    if (lookup == 0) goto RUNCMDFILE;
    if (lookup == -2) return;
    if (*pathptr == ';') {
      pathptr++;
    } else {
      break;
    }
  }

  /* last chance: is it an executable link? (trim extension from cmd first) */
  for (i = 0; (cmd[i] != 0) && (cmd[i] != '.') && (i < 9); i++) buff[128 + i] = cmd[i];
  buff[128 + i] = 0;
  if ((i < 9) && (link_computefname(buff, buff + 128, envseg) == 0)) {
    /* try opening the link file (if it exists) and read it into buff */
    i = 0;
    _asm {
      push ax
      push bx
      push cx
      push dx

      mov ax, 0x3d00  /* DOS 2+ - OPEN EXISTING FILE, READ-ONLY */
      mov dx, buff    /* file name */
      int 0x21
      jc ERR_FOPEN
      /* file handle in AX, read from file now */
      mov bx, ax      /* file handle */
      mov ah, 0x3f    /* Read from file via handle bx */
      mov cx, 128     /* up to 128 bytes */
      /* mov dx, buff */ /* dest buffer (already set) */
      int 0x21        /* read up to 256 bytes from file and write to buff */
      jc ERR_READ
      mov i, ax
      ERR_READ:
      mov ah, 0x3e    /* close file handle in BX */
      int 0x21
      ERR_FOPEN:

      pop dx
      pop cx
      pop bx
      pop ax
    }

    /* did I read anything? */
    if (i != 0) {
      buff[i] = 0;
      /* trim buff at first \n or \r, just in case someone fiddled with the
       * link file using a text editor */
      for (i = 0; (buff[i] != 0) && (buff[i] != '\r') && (buff[i] != '\n'); i++);
      buff[i] = 0;
      /* lookup check */
      if (buff[0] != 0) {
        lookup = lookup_cmd(cmdfile, cmd, buff, &ext);
        if (lookup == 0) goto RUNCMDFILE;
      }
    }
  }

  /* all failed (ie. executable file not found) */
  return;

  RUNCMDFILE:

  /* special handling of batch files */
  if ((ext != NULL) && (imatch(ext, "bat"))) {
    struct batctx far *newbat;

    /* remember the echo flag (in case bat file disables echo, only when starting first bat) */
    if (rmod->bat == NULL) {
      rmod->flags &= ~FLAG_ECHO_BEFORE_BAT;
      if (rmod->flags & FLAG_ECHOFLAG) rmod->flags |= FLAG_ECHO_BEFORE_BAT;
    }

    /* if bat is not called via a CALL, then free the bat-context linked list */
    if ((flags & CALL_FLAG) == 0) rmod_free_bat_llist(rmod);

    /* allocate a new bat context */
    newbat = rmod_fcalloc(sizeof(struct batctx), rmod->rmodseg, "SVBATCTX");
    if (newbat == NULL) {
      nls_outputnl_doserr(8); /* insufficient memory */
      return;
    }

    /* fill the newly allocated batctx structure */
    _fstrcpy(newbat->fname, cmdfile); /* truename of the BAT file */
    newbat->flags = flags & FLAG_STEPBYSTEP;
    /* explode args of the bat file and store them in rmod buff */
    cmd_explode(buff, cmdline, NULL);
    _fmemcpy(newbat->argv, buff, sizeof(newbat->argv));

    /* push the new bat to the top of rmod's linked list */
    newbat->parent = rmod->bat;
    rmod->bat = newbat;

    return;
  }

  /* copy full filename to execute, along with redirected files (if any) */
  _fstrcpy(rmod_execprog, cmdfile);

  /* copy stdin file if a redirection is needed */
  if (redir->stdinfile) {
    char far *farptr = MK_FP(rmod->rmodseg, RMOD_OFFSET_STDINFILE);
    char far *delstdin = MK_FP(rmod->rmodseg, RMOD_OFFSET_STDIN_DEL);
    _fstrcpy(farptr, redir->stdinfile);
    if (flags & DELETE_STDIN_FILE) {
      *delstdin = redir->stdinfile[0];
    } else {
      *delstdin = 0;
    }
  }

  /* same for stdout file */
  if (redir->stdoutfile) {
    char far *farptr = MK_FP(rmod->rmodseg, RMOD_OFFSET_STDOUTFILE);
    unsigned short far *farptr16 = MK_FP(rmod->rmodseg, RMOD_OFFSET_STDOUTAPP);
    _fstrcpy(farptr, redir->stdoutfile);
    /* openflag */
    *farptr16 = redir->stdout_openflag;
  }

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
  unsigned short batname_seg = FP_SEG(rmod->bat->fname);
  unsigned short batname_off = FP_OFF(rmod->bat->fname);
  unsigned short filepos_cx = rmod->bat->nextline >> 16;
  unsigned short filepos_dx = rmod->bat->nextline & 0xffff;
  unsigned char blen = 0;
  unsigned short errv = 0;

  /* open file, jump to offset filpos, and read data into buff.
   * result in blen (unchanged if EOF or failure). */
  _asm {
    push ax
    push bx
    push cx
    push dx

    /* open file (read-only) */
    mov bx, 0xffff        /* preset BX to 0xffff to detect error conditions */
    mov dx, batname_off
    mov ax, batname_seg
    push ds     /* save DS */
    mov ds, ax
    mov ax, 0x3d00
    int 0x21    /* handle in ax on success */
    pop ds      /* restore DS */
    jc ERR
    mov bx, ax  /* save handle to bx */

    /* jump to file offset CX:DX */
    mov ax, 0x4200
    mov cx, filepos_cx
    mov dx, filepos_dx
    int 0x21  /* CF clear on success, DX:AX set to cur pos */
    jc ERR

    /* read the line into buff */
    mov ah, 0x3f
    xor ch, ch
    mov cl, buffmaxlen
    mov dx, buff
    int 0x21 /* CF clear on success, AX=number of bytes read */
    jc ERR
    mov blen, al
    jmp CLOSEANDQUIT

    ERR:
    mov errv, ax

    CLOSEANDQUIT:
    /* close file (if bx contains a handle) */
    cmp bx, 0xffff
    je DONE
    mov ah, 0x3e
    int 0x21

    DONE:
    pop dx
    pop cx
    pop bx
    pop ax
  }

  /* printf("blen=%u filepos_cx=%u filepos_dx=%u\r\n", blen, filepos_cx, filepos_dx); */

  if (errv != 0) nls_outputnl_doserr(errv);

  /* on EOF - abort processing the bat file */
  if (blen == 0) goto OOPS;

  /* find nearest \n to inc batch offset and replace \r by NULL terminator
   * I support all CR/LF, CR- and LF-terminated batch files */
  for (i = 0; i < blen; i++) {
    if ((buff[i] == '\r') || (buff[i] == '\n')) {
      if ((buff[i] == '\r') && ((i+1) < blen) && (buff[i+1] == '\n')) rmod->bat->nextline += 1;
      break;
    }
  }
  buff[i] = 0;
  rmod->bat->nextline += i + 1;

  return(0);

  OOPS:
  rmod->bat->fname[0] = 0;
  rmod->bat->nextline = 0;
  return(-1);
}


/* replaces %-variables in a BAT line with resolved values:
 * %PATH%       -> replaced by the contend of the PATH env variable
 * %UNDEFINED%  -> undefined variables are replaced by nothing ("")
 * %NOTCLOSED   -> NOTCLOSED
 * %1           -> first argument of the batch file (or nothing if no arg) */
static void batpercrepl(char *res, unsigned short ressz, const char *line, const struct rmod_props far *rmod, unsigned short envseg) {
  unsigned short lastperc = 0xffff;
  unsigned short reslen = 0;

  if (ressz == 0) return;
  ressz--; /* reserve one byte for the NULL terminator */

  for (; (reslen < ressz) && (*line != 0); line++) {
    /* if not a percent, I don't care */
    if (*line != '%') {
      res[reslen++] = *line;
      continue;
    }

    /* *** perc char handling *** */

    /* closing perc? */
    if (lastperc != 0xffff) {
      /* %% is '%' */
      if (lastperc == reslen) {
        res[reslen++] = '%';
      } else {   /* otherwise variable name */
        const char far *ptr;
        res[reslen] = 0;
        reslen = lastperc;
        strupr(res + reslen); /* turn varname uppercase before lookup */
        ptr = env_lookup_val(envseg, res + reslen);
        if (ptr != NULL) {
          while ((*ptr != 0) && (reslen < ressz)) {
            res[reslen++] = *ptr;
            ptr++;
          }
        }
      }
      lastperc = 0xffff;
      continue;
    }

    /* digit? (bat arg) */
    if ((line[1] >= '0') && (line[1] <= '9')) {
      unsigned short argid = line[1] - '0';
      unsigned short i;
      const char far *argv = "";
      if ((rmod != NULL) && (rmod->bat != NULL)) argv = rmod->bat->argv;

      /* locate the proper arg */
      for (i = 0; i != argid; i++) {
        /* if string is 0, then end of list reached */
        if (*argv == 0) break;
        /* jump to next arg */
        while (*argv != 0) argv++;
        argv++;
      }

      /* copy the arg to result */
      for (i = 0; (argv[i] != 0) && (reslen < ressz); i++) {
        res[reslen++] = argv[i];
      }
      line++;  /* skip the digit */
      continue;
    }

    /* opening perc */
    lastperc = reslen;

  }

  res[reslen] = 0;
}


/* process the ongoing forloop, returns 0 on success, non-zero otherwise (no
   more things to process) */
static int forloop_process(char *res, struct forctx far *forloop) {
  unsigned short i, t;
  struct DTA *dta = (void *)0x80; /* default DTA at 80h in PSP */
  char *fnameptr = dta->fname;

  TRYAGAIN:

  /* dta_inited: FindFirst() or FindNext()? */
  if (forloop->dta_inited == 0) {

    /* copy next awaiting pattern to BUFFER (and skip all delimieters until
     * next pattern or end of list) */
    t = 0;
    for (i = 0;; i++) {
      BUFFER[i] = forloop->cmd[forloop->nextpat + i];
      /* look for a delimiter - the list of valid delimiters has been
       * researched and kindly shared by Robert Riebisch via the ticket
       * at https://osdn.net/projects/svardos/ticket/44058 */
      switch (BUFFER[i]) {
        case ' ':
        case '\t':
        case ';':
        case ',':
        case '/':
        case '=':
          BUFFER[i] = 0;
          t = 1;
          break;
        case 0: /* end of patterns list */
          t = 2;
          break;
        default: /* quit if I got a pattern already */
          if (t == 1) t = 2;
          break;
      }
      if (t == 2) break;
    }

    if (i == 0) return(-1);

    /* remember position of current pattern */
    forloop->curpat = forloop->nextpat;

    /* move nextpat forward to next pattern */
    i += forloop->nextpat;
    forloop->nextpat = i;

    /* if this is a string and not a pattern, skip all the FindFirst business
     * a file pattern has a wildcard (* or ?), a message doesn't */
    for (i = 0; (BUFFER[i] != 0) && (BUFFER[i] != '?') && (BUFFER[i] != '*'); i++);
    if (BUFFER[i] == 0) {
      fnameptr = BUFFER;
      goto SKIP_DTA;
    }

    /* FOR in MSDOS 6 includes hidden and system files, but not directories nor volumes */
    if (findfirst(dta, BUFFER, DOS_ATTR_RO | DOS_ATTR_HID | DOS_ATTR_SYS | DOS_ATTR_ARC) != 0) {
      goto TRYAGAIN;
    }
    forloop->dta_inited = 1;
  } else { /* dta in progress */

    /* copy forloop DTA to my local copy */
    _fmemcpy(dta, &(forloop->dta), sizeof(*dta));

    /* findnext() call */
    if (findnext(dta) != 0) {
      forloop->dta_inited = 0;
      goto TRYAGAIN;
    }
  }

  /* copy updated DTA to rmod */
  _fmemcpy(&(forloop->dta), dta, sizeof(*dta));

  SKIP_DTA:

  /* fill res with command, replacing varname by actual filename */
  /* full filename is to be built with path of curpat and fname from dta */
  t = 0;
  i = 0;
  for (;;) {
    if ((forloop->cmd[forloop->exec + t] == '%') && (forloop->cmd[forloop->exec + t + 1] == forloop->varname)) {
      short lastbk = i;
      char far *c = forloop->cmd + forloop->curpat;
      for (;;) {
        res[i] = *c;
        c++;
        if (res[i] == '\\') lastbk = i;
        if ((res[i] == ' ') || (res[i] == 0)) break;
        i++;
      }
      strcpy(res + lastbk, fnameptr);
      for (i = lastbk; res[i] != 0; i++);
      t += 2;
    } else {
      res[i] = forloop->cmd[forloop->exec + t];
      t++;
      if (res[i++] == 0) break;
    }
  }

  return(0);
}


int main(void) {
  static struct config cfg;
  static unsigned short far *rmod_envseg;
  static unsigned short far *lastexitcode;
  static struct rmod_props far *rmod;
  static char cmdlinebuf[CMDLINE_MAXLEN + 2]; /* 1 extra byte for 0-terminator and another for memguard */
  static char *cmdline;
  static struct redir_data redirprops;
  static enum cmd_result cmdres;
  static unsigned short i; /* general-purpose variable for short-lived things */
  static unsigned char flags;

  rmod = rmod_find(BUFFER_len);
  if (rmod == NULL) {
    /* look at command line parameters (in case env size if set there) */
    parse_argv(&cfg);
    rmod = rmod_install(cfg.envsiz, BUFFER, BUFFER_len);
    if (rmod == NULL) {
      nls_outputnl_err(2,1); /* "FATAL ERROR: rmod_install() failed" */
      return(1);
    }
    /* copy flags to rmod's storage (and enable ECHO) */
    rmod->flags = cfg.flags | FLAG_ECHOFLAG;
    /* printf("rmod installed at %Fp\r\n", rmod); */
    rmod->version = BYTE_VERSION;
  } else {
    /* printf("rmod found at %Fp\r\n", rmod); */
    /* if I was spawned by rmod and FLAG_EXEC_AND_QUIT is set, then I should
     * die asap, because the command has been executed already, so I no longer
     * have a purpose in life */
    if (rmod->flags & FLAG_EXEC_AND_QUIT) sayonara(rmod);
    /* */
    if (rmod->version != BYTE_VERSION) {
      nls_outputnl_err(2,0);
      _asm {
        HALT:
        hlt
        jmp HALT
      }
    }
  }

  /* install a few guardvals in memory to detect some cases of overflows */
  memguard_set(cmdlinebuf);

  rmod_envseg = MK_FP(rmod->rmodseg, RMOD_OFFSET_ENVSEG);
  lastexitcode = MK_FP(rmod->rmodseg, RMOD_OFFSET_LEXITCODE);

  /* make COMPSEC point to myself */
  set_comspec_to_self(*rmod_envseg);

  /* on /P check for the presence of AUTOEXEC.BAT and execute it if found,
   * but skip this check if /D was also passed */
  if ((cfg.flags & (FLAG_PERMANENT | FLAG_SKIP_AUTOEXEC)) == FLAG_PERMANENT) {
    if (file_getattr("AUTOEXEC.BAT") >= 0) cfg.execcmd = "AUTOEXEC.BAT";
  }

  do {

    /* terminate previous command with a CR/LF if ECHO ON (but not during BAT processing) */
    if ((rmod->flags & FLAG_ECHOFLAG) && (rmod->bat == NULL)) outputnl("");

    SKIP_NEWLINE:

    /* memory check */
    memguard_check(rmod->rmodseg, cmdlinebuf);

    /* preset cmdline to point at the dedicated buffer */
    cmdline = cmdlinebuf;

    /* (re)load translation strings if needed */
    nls_langreload(BUFFER, *rmod_envseg);

    /* am I inside a FOR loop? */
    if (rmod->forloop) {
      if (forloop_process(cmdlinebuf, rmod->forloop) != 0) {
        rmod_ffree(rmod->forloop);
        rmod->forloop = NULL;
      } else {
        /* output prompt and command on screen if echo on and command is not
         * inhibiting it with the @ prefix */
        if (rmod->flags & FLAG_ECHOFLAG) {
          build_and_display_prompt(BUFFER, *rmod_envseg);
          outputnl(cmdline);
        }
        /* jump to command processing */
        goto EXEC_CMDLINE;
      }
    }

    /* load awaiting command, if any (used to run piped commands) */
    if (rmod->awaitingcmd[0] != 0) {
      _fstrcpy(cmdline, rmod->awaitingcmd);
      rmod->awaitingcmd[0] = 0;
      flags |= DELETE_STDIN_FILE;
      goto EXEC_CMDLINE;
    } else {
      flags &= ~DELETE_STDIN_FILE;
    }

    /* skip user input if I have a command to exec (/C or /K or /P) */
    if (cfg.execcmd != NULL) {
      cmdline = cfg.execcmd;
      cfg.execcmd = NULL;
      /* */
      if (cfg.flags & FLAG_STEPBYSTEP) flags |= FLAG_STEPBYSTEP;
      goto EXEC_CMDLINE;
    }

    /* if batch file is being executed -> fetch next line */
    if (rmod->bat != NULL) {
      if (getbatcmd(BUFFER, CMDLINE_MAXLEN, rmod) != 0) { /* end of batch */
        struct batctx far *victim = rmod->bat;
        rmod->bat = rmod->bat->parent;
        rmod_ffree(victim);
        /* end of batch? then restore echo flag as it was before running the (first) bat file */
        if (rmod->bat == NULL) {
          rmod->flags &= ~FLAG_ECHOFLAG;
          if (rmod->flags & FLAG_ECHO_BEFORE_BAT) rmod->flags |= FLAG_ECHOFLAG;
        }
        continue;
      }
      /* %-decoding of variables (%PATH%, %1, %%...), result in cmdline */
      batpercrepl(cmdline, CMDLINE_MAXLEN, BUFFER, rmod, *rmod_envseg);
      /* skip any leading spaces */
      while (*cmdline == ' ') cmdline++;
      /* skip batch labels */
      if (*cmdline == ':') continue;
      /* step-by-step execution? */
      if (rmod->bat->flags & FLAG_STEPBYSTEP) {
        if (*cmdline == 0) continue; /* skip empty lines */
        if (askchoice(cmdline, svarlang_str(0,10)) != 0) continue;
      }
      /* output prompt and command on screen if echo on and command is not
       * inhibiting it with the @ prefix */
      if ((rmod->flags & FLAG_ECHOFLAG) && (cmdline[0] != '@')) {
        build_and_display_prompt(BUFFER, *rmod_envseg);
        outputnl(cmdline);
      }
      /* skip the @ prefix if present, it is no longer useful */
      if (cmdline[0] == '@') cmdline++;
    } else {
      unsigned char far *rmod_inputbuf = MK_FP(rmod->rmodseg, RMOD_OFFSET_INPUTBUF);
      /* invalidate input history if it appears to be damaged (could occur
       * because of a stack overflow, for example if some stack-hungry TSR is
       * being used) */
      if ((rmod_inputbuf[0] != 128) || (rmod_inputbuf[rmod_inputbuf[1] + 2] != '\r') || (rmod_inputbuf[rmod_inputbuf[1] + 3] != 0xCA) || (rmod_inputbuf[rmod_inputbuf[1] + 4] != 0xFE)) {
        rmod_inputbuf[0] = 128;  /* max allowed input length */
        rmod_inputbuf[1] = 0;    /* string len stored in buffer */
        rmod_inputbuf[2] = '\r'; /* string terminator */
        rmod_inputbuf[3] = 0xCA; /* trailing signature */
        rmod_inputbuf[4] = 0xFE; /* trailing signature */
        nls_outputnl_err(2,2); /* "stack overflow detected, command history flushed" */
      }
      /* interactive mode: display prompt (if echo enabled) and wait for user
       * command line */
      if (rmod->flags & FLAG_ECHOFLAG) build_and_display_prompt(BUFFER, *rmod_envseg);
      /* collect user input */
      cmdline_getinput(rmod->rmodseg, RMOD_OFFSET_INPUTBUF);
      /* append stack-overflow detection signature to the end of the input buffer */
      rmod_inputbuf[rmod_inputbuf[1] + 3] = 0xCA; /* trailing signature */
      rmod_inputbuf[rmod_inputbuf[1] + 4] = 0xFE; /* trailing signature */
      /* copy it to local cmdline */
      if (rmod_inputbuf[1] != 0) _fmemcpy(cmdline, rmod_inputbuf + 2, rmod_inputbuf[1]);
      cmdline[rmod_inputbuf[1]] = 0; /* zero-terminate local buff (original is '\r'-terminated) */
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
    i = redir_parsecmd(&redirprops, cmdline, rmod->awaitingcmd, *rmod_envseg);
    if (i != 0) {
      nls_outputnl_doserr(i);
      rmod->awaitingcmd[0] = 0;
      continue;
    }

    /* try matching (and executing) an internal command */
    cmdres = cmd_process(rmod, *rmod_envseg, cmdline, BUFFER, sizeof(BUFFER), &redirprops, flags & DELETE_STDIN_FILE);
    if ((cmdres == CMD_OK) || (cmdres == CMD_FAIL)) {
      /* internal command executed */
    } else if (cmdres == CMD_CHANGED) { /* cmdline changed, needs to be reprocessed */
      goto EXEC_CMDLINE;
    } else if (cmdres == CMD_CHANGED_BY_CALL) { /* cmdline changed *specifically* by CALL */
      /* the distinction is important since it changes the way batch files are processed */
      flags |= CALL_FLAG;
      goto EXEC_CMDLINE;
    } else if (cmdres == CMD_NOTFOUND) {
      /* this was not an internal command, try matching an external command */
      run_as_external(BUFFER, cmdline, *rmod_envseg, rmod, &redirprops, flags);

      /* is it a newly launched BAT file? */
      if ((rmod->bat != NULL) && (rmod->bat->nextline == 0)) goto SKIP_NEWLINE;
      /* run_as_external() does not return on success, if I am still alive then
       * external command failed to execute */
      nls_outputnl(0,5); /* "Bad command or file name" */
    } else {
      /* I should never ever land here */
      outputnl("INTERNAL ERR: INVALID CMDRES");
    }

    /* reset one-time only flags */
    flags &= ~CALL_FLAG;
    flags &= ~FLAG_STEPBYSTEP;

    /* repeat unless /C was asked - but always finish running an ongoing batch
     * file (otherwise only first BAT command would be executed with /C) */
  } while (((rmod->flags & FLAG_EXEC_AND_QUIT) == 0) || (rmod->bat != NULL) || (rmod->forloop != NULL));

  sayonara(rmod);
  return(0);
}
