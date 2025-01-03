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

#ifndef RMODINIT_H
#define RMODINIT_H

#include "helpers.h" /* struct DTA definition */

#define FLAG_EXEC_AND_QUIT    1
#define FLAG_PERMANENT        2
#define FLAG_ECHOFLAG         4
#define FLAG_ECHO_BEFORE_BAT  8
#define FLAG_SKIP_AUTOEXEC   16
#define FLAG_STEPBYSTEP      32
#define FLAG_MSG             64


/* batch context structure used to track what batch file is being executed,
 * at what line, arguments, whether or not it has a parent batch... */
struct batctx {
  char fname[130];            /* truename of batch file being processed */
  char argv[130];             /* args of the batch call (0-separated) */
  unsigned char flags;        /* used for step-by-step execution */
  unsigned long nextline;     /* offset in file of next bat line to process */
  struct batctx far *parent;  /* parent context if this batch was CALLed */
};

/* for context structure used to track the execution of the ongoing FOR loop */
struct forctx {
  char cmd[130];              /* copy of the original FOR command */
  struct DTA dta;             /* DTA for FindNext on current pattern */
  unsigned short curpat;      /* cmd offset of currently processed pattern */
  unsigned short nextpat;     /* cmd offset of next pattern to be processed */
  unsigned short exec;        /* cmd offset of the command to be executed */
  char varname;               /* single char of variable name */
  unsigned char dta_inited;   /* 0=requires FindFirst 1=FindNext */
};

struct rmod_props {
  unsigned short rmodseg;     /* segment where rmod is loaded */
  unsigned long origint22;    /* original int 22 handler (far ptr) of the shell */
  unsigned short origparent;  /* original parent (PSP's segment) */
  unsigned char flags;        /* command line parameters */
  unsigned char version;      /* used to detect mismatch between rmod and SvarCOM */
  char awaitingcmd[130];      /* command to exec next time (if any) */
  struct batctx far *bat;     /* linked list of bat contexts, if BAT ongoing */
  struct forctx far *forloop; /* a single FOR loop structure, if FOR ongoing */
  unsigned short lng_segmem;  /* segment where a lang block copy is kept (/M), 0=none */
  unsigned short lng_segdict; /* segment where a lang block copy is kept (/M), 0=none */
  unsigned short lng_current; /* current LNG loaded in lng_seg* (0 = none) */
};

#define RMOD_OFFSET_ENVSEG     0x2C   /* stored in rmod's PSP */
#define RMOD_OFFSET_INPUTBUF   (0x100 + 0x08)
#define RMOD_OFFSET_COMSPECPTR (0x100 + 0xCE)
#define RMOD_OFFSET_BOOTDRIVE  (0x100 + 0xD0)
#define RMOD_OFFSET_LEXITCODE  (0x100 + 0xDF)
#define RMOD_OFFSET_EXECPARAM  (0x100 + 0xE0)
#define RMOD_OFFSET_EXECPROG   (0x100 + 0xEE)
#define RMOD_OFFSET_STDINFILE  (0x100 + 0x16E)
#define RMOD_OFFSET_STDOUTFILE (0x100 + 0x1EE)
#define RMOD_OFFSET_STDOUTAPP  (0x100 + 0x26E)
#define RMOD_OFFSET_STDIN_DEL  (0x100 + 0x270)
#define RMOD_OFFSET_EXEC_LH    (0x100 + 0x271)
#define RMOD_OFFSET_ORIGUMBLNK (0x100 + 0x272)
#define RMOD_OFFSET_ORIGALLOCS (0x100 + 0x273)
#define RMOD_OFFSET_CTRLCFLAG  (0x100 + 0x274)
#define RMOD_OFFSET_BRKHANDLER (0x100 + 0x275)
#define RMOD_OFFSET_ROUTINE    (0x100 + 0x27F)
#define RMOD_OFFSET_CRITHANDLER (0x100 + 0x464)
#define RMOD_OFFSET_CRITMSG    (0x100 + 0x56A)

struct rmod_props far *rmod_install(unsigned short envsize, unsigned char *rmodcore, unsigned short rmodcore_len, unsigned char *cfgflags);
struct rmod_props far *rmod_find(unsigned short rmodcore_len);
void rmod_updatecomspecptr(unsigned short rmod_seg, unsigned short env_seg);

/* allocates bytes of far memory, flags it as belonging to rmod
 * the new block can be optionally flagged as 'ident' (if not null) and zero
 * out the newly allocated memory.
 * returns a far ptr to the allocated block, or NULL on error */
void far *rmod_fcalloc(unsigned short bytes, unsigned short rmod_seg, char *ident);

/* free memory previously allocated by rmod_fcalloc() */
void rmod_ffree(void far *ptr);

/* free the entire linked list of bat ctx nodes (and set its rmod ptr to NULL) */
void rmod_free_bat_llist(struct rmod_props far *rmod);

#endif
