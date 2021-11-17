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


struct config {
  unsigned char flags; /* command.com flags, as defined in rmodinit.h */
  char *execcmd;
  unsigned short envsiz;
};


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

        case 'p': /* permanent shell (can't exit) */
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


static void buildprompt(char *s, unsigned short envseg) {
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
  *s = '$';
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


static void run_as_external(char *buff, const char far *cmdline, unsigned short envseg, struct rmod_props far *rmod) {
  char const **argvlist = (void *)(buff + 512);
  char *cmdfile = buff + 1024;
  const char far *pathptr;
  int lookup;
  unsigned short i;
  const char *ext;
  const char far *cmdtail;
  char far *rmod_execprog = MK_FP(rmod->rmodseg, RMOD_OFFSET_EXECPROG);
  char far *rmod_cmdtail = MK_FP(rmod->rmodseg, 0x81);
  _Packed struct {
    unsigned short envseg;
    unsigned long cmdtail;
    unsigned long fcb1;
    unsigned long fcb2;
  } far *ExecParam = MK_FP(rmod->rmodseg, RMOD_OFFSET_EXECPARAM);

  cmd_explode(buff + 2048, cmdline, argvlist);

  /* for (i = 0; argvlist[i] != NULL; i++) printf("arg #%d = '%s'\r\n", i, argvlist[i]); */

  /* is this a command in curdir? */
  lookup = lookup_cmd(cmdfile, argvlist[0], NULL, &ext);
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
    lookup = lookup_cmd(cmdfile, argvlist[0], buff, &ext);
    if (lookup == 0) break;
    if (lookup == -2) return;
    if (*pathptr == ';') {
      pathptr++;
    } else {
      return;
    }
  }

  RUNCMDFILE:

  /* find cmdtail */
  cmdtail = cmdline;
  while (*cmdtail == ' ') cmdtail++;
  while ((*cmdtail != ' ') && (*cmdtail != '/') && (*cmdtail != '+') && (*cmdtail != 0)) {
    cmdtail++;
  }

  /* special handling of batch files */
  if ((ext != NULL) && (imatch(ext, "bat"))) {
    /* copy truename of the bat file to rmod buff */
    for (i = 0; cmdfile[i] != 0; i++) rmod->batfile[i] = cmdfile[i];
    rmod->batfile[i] = 0;
    /* copy args of the bat file to rmod buff */
    for (i = 0; cmdtail[i] != 0; i++) rmod->batargs[i] = cmdtail[i];
    /* reset the 'next line to execute' counter */
    rmod->batnextline = 0;
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


/* fetches a line from batch file and write it to buff, increments
 * rmod counter on success. returns NULL on failure.
 * buff must start with a length byte but the returned pointer must
 * skip it. */
char far *getbatcmd(char far *buff, struct rmod_props far *rmod) {
  unsigned short i;
  buff++; /* make room for the len byte */
  /* TODO temporary hack to display a dummy message */
  if (rmod->batnextline == 0) {
    char *msg = "ECHO batch files not supported yet";
    for (i = 0; msg[i] != 0; i++) buff[i] = msg[i];
    buff[i] = 0;
    buff[-1] = i;
  } else {
    rmod->batfile[0] = 0;
    return(NULL);
  }
  /* open file */
  /* read until awaiting line */
  /* copy line to buff */
  /* close file */
  /* */
  rmod->batnextline++;
  if (rmod->batnextline == 0) rmod->batfile[0] = 0; /* max line count reached */
  return(buff);
}


int main(void) {
  static struct config cfg;
  static unsigned short far *rmod_envseg;
  static unsigned short far *lastexitcode;
  static unsigned char BUFFER[4096];
  static struct rmod_props far *rmod;

  rmod = rmod_find();
  if (rmod == NULL) {
    rmod = rmod_install(cfg.envsiz);
    if (rmod == NULL) {
      outputnl("ERROR: rmod_install() failed");
      return(1);
    }
    /* look at command line parameters */
    parse_argv(&cfg);
    /* copy flags to rmod's storage */
    rmod->flags = cfg.flags;
    /* printf("rmod installed at %Fp\r\n", rmod); */
  } else {
    /* printf("rmod found at %Fp\r\n", rmod); */
    /* if I was spawned by rmod and FLAG_EXEC_AND_QUIT is set, then I should
     * die asap, because the command has been executed already, so I no longer
     * have a purpose in life */
    if (rmod->flags & FLAG_EXEC_AND_QUIT) sayonara(rmod);
  }

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

  do {
    char far *cmdline = rmod->inputbuf + 2;

    /* (re)load translation strings if needed */
    nls_langreload(BUFFER, *rmod_envseg);

    /* skip user input if I have a command to exec (/C or /K) */
    if (cfg.execcmd != NULL) {
      cmdline = cfg.execcmd;
      cfg.execcmd = NULL;
      goto EXEC_CMDLINE;
    }

    if (rmod->echoflag != 0) outputnl(""); /* terminate the previous command with a CR/LF */

    SKIP_NEWLINE:

    /* print shell prompt (only if ECHO is enabled) */
    if (rmod->echoflag != 0) {
      char *promptptr = BUFFER;
      buildprompt(promptptr, *rmod_envseg);
      _asm {
        push ax
        push dx
        mov ah, 0x09
        mov dx, promptptr
        int 0x21
        pop dx
        pop ax
      }
    }

    /* revert input history terminator to \r */
    if (cmdline[-1] != 0) {
      cmdline[(unsigned short)(cmdline[-1])] = '\r';
    }

    /* if batch file is being executed -> fetch next line */
    if (rmod->batfile[0] != 0) {
      cmdline = getbatcmd(BUFFER + sizeof(BUFFER) - 130, rmod);
      if (cmdline == NULL) continue;
    } else {
      /* interactive mode: wait for user command line */
      cmdline_getinput(FP_SEG(rmod->inputbuf), FP_OFF(rmod->inputbuf));
    }

    /* if nothing entered, loop again (but without appending an extra CR/LF) */
    if (cmdline[-1] == 0) goto SKIP_NEWLINE;

    /* replace \r by a zero terminator */
    cmdline[(unsigned char)(cmdline[-1])] = 0;

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

    /* revert stdout (in case it was redirected) */
    redir_revert();

    /* run_as_external() does not return on success, if I am still alive then
     * external command failed to execute */
    outputnl("Bad command or file name");

  } while ((rmod->flags & FLAG_EXEC_AND_QUIT) == 0);

  sayonara(rmod);
  return(0);
}
