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
      output("Invalid parameter: ");
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
        cfg->execcmd = cmdline + 1;
        return;

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
        outputnl("Starts the SvarCOM command interpreter");
        outputnl("");
        outputnl("COMMAND /E:nnn [/[C|K] [/P] [/D] command]");
        outputnl("");
        outputnl("/D      Skip AUTOEXEC.BAT processing (makes sense only with /P)");
        outputnl("/E:nnn  Sets the environment size to nnn bytes");
        outputnl("/P      Makes the new command interpreter permanent and run AUTOEXEC.BAT");
        outputnl("/C      Executes the specified command and returns");
        outputnl("/K      Executes the specified command and continues running");
        exit(1);
        break;

      default:
        output("Invalid switch: /");
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


/* tries locating executable fname in path and fill res with result. returns 0 on success,
 * -1 on failed match and -2 on failed match + "don't even try with other paths"
 * extptr contains a ptr to the extension in fname (NULL if not found) */
static int lookup_cmd(char *res, const char *fname, const char *path, const char **extptr) {
  unsigned short lastbslash = 0;
  unsigned short i, len;
  unsigned char explicitpath = 0;

  /* does the original fname has an explicit path prefix or explicit ext? */
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

  /* if no path prefix was found in fname (no colon or backslash) AND we have
   * a path arg, then assemble path+filename */
  if ((!explicitpath) && (path != NULL) && (path[0] != 0)) {
    i = strlen(path);
    if (path[i - 1] != '\\') i++; /* add a byte for inserting a bkslash after path */
    /* move the filename at the place where path will end */
    memmove(res + i, res + lastbslash + 1, len - lastbslash);
    /* copy path in front of the filename and make sure there is a bkslash sep */
    memmove(res, path, i);
    res[i - 1] = '\\';
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


static void run_as_external(char *buff, const char *cmdline, unsigned short envseg, struct rmod_props far *rmod, struct redir_data *redir) {
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
    _fstrcpy(rmod->batfile, cmdfile);

    /* explode args of the bat file and store them in rmod buff */
    cmd_explode(buff, cmdline, NULL);
    _fmemcpy(rmod->batargv, buff, sizeof(rmod->batargv));

    /* reset the 'next line to execute' counter */
    rmod->batnextline = 0;
    /* remember the echo flag (in case bat file disables echo) */
    rmod->flags &= ~FLAG_ECHO_BEFORE_BAT;
    if (rmod->flags & FLAG_ECHOFLAG) rmod->flags |= FLAG_ECHO_BEFORE_BAT;
    return;
  }

  /* copy full filename to execute, along with redirected files (if any) */
  _fstrcpy(rmod_execprog, cmdfile);

  /* copy stdin file if a redirection is needed */
  if (redir->stdinfile) {
    char far *farptr = MK_FP(rmod->rmodseg, RMOD_OFFSET_STDINFILE);
    _fstrcpy(farptr, redir->stdinfile);
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
  unsigned short batname_seg = FP_SEG(rmod->batfile);
  unsigned short batname_off = FP_OFF(rmod->batfile);
  unsigned short filepos_cx = rmod->batnextline >> 16;
  unsigned short filepos_dx = rmod->batnextline & 0xffff;
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
      const char far *argv = rmod->batargv;

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
  memguard_set(cmdlinebuf);

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

  /* on /P check for the presence of AUTOEXEC.BAT and execute it if found,
   * but skip this check if /D was also passed */
  if ((cfg.flags & (FLAG_PERMANENT | FLAG_SKIP_AUTOEXEC)) == FLAG_PERMANENT) {
    if (file_getattr("AUTOEXEC.BAT") >= 0) cfg.execcmd = "AUTOEXEC.BAT";
  }

  do {
    /* terminate previous command with a CR/LF if ECHO ON (but not during BAT processing) */
    if ((rmod->flags & FLAG_ECHOFLAG) && (rmod->batfile[0] == 0)) outputnl("");

    SKIP_NEWLINE:

    /* cancel any redirections that may have been set up before */
    redir_revert();

    /* memory check */
    memguard_check(rmod->rmodseg, cmdlinebuf);

    /* preset cmdline to point at the dedicated buffer */
    cmdline = cmdlinebuf;

    /* (re)load translation strings if needed */
    nls_langreload(BUFFER, *rmod_envseg);

    /* load awaiting command, if any (used to run piped commands) */
    if (rmod->awaitingcmd[0] != 0) {
      _fstrcpy(cmdline, rmod->awaitingcmd);
      rmod->awaitingcmd[0] = 0;
      goto EXEC_CMDLINE;
    }

    /* skip user input if I have a command to exec (/C or /K) */
    if (cfg.execcmd != NULL) {
      cmdline = cfg.execcmd;
      cfg.execcmd = NULL;
      goto EXEC_CMDLINE;
    }

    /* if batch file is being executed -> fetch next line */
    if (rmod->batfile[0] != 0) {
      if (getbatcmd(BUFFER, CMDLINE_MAXLEN, rmod) != 0) { /* end of batch */
        /* restore echo flag as it was before running the bat file */
        rmod->flags &= ~FLAG_ECHOFLAG;
        if (rmod->flags & FLAG_ECHO_BEFORE_BAT) rmod->flags |= FLAG_ECHOFLAG;
        continue;
      }
      /* %-decoding of variables (%PATH%, %1, %%...), result in cmdline */
      batpercrepl(cmdline, CMDLINE_MAXLEN, BUFFER, rmod, *rmod_envseg);
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
    i = redir_parsecmd(&redirprops, cmdline, rmod->awaitingcmd);
    if (i != 0) {
      nls_outputnl_doserr(i);
      rmod->awaitingcmd[0] = 0;
      continue;
    }

    /* try matching (and executing) an internal command */
    cmdres = cmd_process(rmod, *rmod_envseg, cmdline, BUFFER, sizeof(BUFFER), &redirprops);
    if ((cmdres == CMD_OK) || (cmdres == CMD_FAIL)) {
      /* internal command executed */
      continue;
    } else if (cmdres == CMD_CHANGED) { /* cmdline changed, needs to be reprocessed */
      goto EXEC_CMDLINE;
    } else if (cmdres == CMD_NOTFOUND) {
      /* this was not an internal command, try matching an external command */
      run_as_external(BUFFER, cmdline, *rmod_envseg, rmod, &redirprops);
      /* perhaps this is a newly launched BAT file */
      if ((rmod->batfile[0] != 0) && (rmod->batnextline == 0)) goto SKIP_NEWLINE;
      /* run_as_external() does not return on success, if I am still alive then
       * external command failed to execute */
      outputnl("Bad command or file name");
      continue;
    }

    /* I should never ever land here */
    outputnl("INTERNAL ERR: INVALID CMDRES");

  } while ((rmod->flags & FLAG_EXEC_AND_QUIT) == 0);

  sayonara(rmod);
  return(0);
}
