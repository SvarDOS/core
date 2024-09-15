/* This file is part of the svarlang project and is published under the terms
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

/* if WITHSTDIO is enabled, then remap file operations to use the standard
 * stdio amenities */
#ifdef WITHSTDIO

#include <stdio.h>   /* FILE, fopen(), fseek(), etc */
typedef FILE* FHANDLE;
#define FOPEN(x) fopen(x, "rb")
#define FCLOSE(x) fclose(x)
#define FSEEK(f,b) fseek(f,b,SEEK_CUR)
#define FREAD(f,t,b) fread(t, 1, b, f)

#else

#include <i86.h>  /* FP_SEG, FP_OFF */
typedef unsigned short FHANDLE;

#endif


#include "svarlang.h"


/* supplied through DEFLANG.C */
extern char svarlang_mem[];
extern unsigned short svarlang_dict[];
extern const unsigned short svarlang_memsz;
extern const unsigned short svarlang_string_count;


const char *svarlang_strid(unsigned short id) {
  unsigned short left = 0, right = svarlang_string_count - 1, x;
  unsigned short v;

  if (svarlang_string_count == 0) return("");

  while (left <= right) {
    x = left + ((right - left ) >> 2);
    v = svarlang_dict[x * 2];

    if (id == v) return(svarlang_mem + svarlang_dict[x * 2 + 1]);

    if (id > v) {
      if (left < 65535) left = x + 1;
      else goto not_found;
    } else {
      if (right > 0) right = x - 1;
      else goto not_found;
    }
  }

not_found:
  return("");
}


/* routines below are simplified (dos-based) versions of the libc FILE-related
 * functions. Using them avoids a dependency on FILE, hence makes the binary
 * smaller if the application does not need to pull fopen() and friends
 * I use pragma aux directives for more compact size. open-watcom only. */
#ifndef WITHSTDIO

static unsigned short FOPEN(const char far *s);

#pragma aux FOPEN = \
"push ds" \
"push es" \
"pop ds" \
"mov ax, 0x3D00" /* open file, read-only (fname at DS:DX) */ \
"int 0x21" \
"jnc DONE" \
"xor ax, ax"     /* return 0 on error */ \
"DONE:" \
"pop ds" \
parm [es dx] \
value [ax];


static void FCLOSE(unsigned short handle);

#pragma aux FCLOSE = \
"mov ah, 0x3E" \
"int 0x21" \
modify [ax]  /* AX might contain an error code on failure */ \
parm [bx]


static unsigned short FREAD(unsigned short handle, void far *buff, unsigned short bytes);

#pragma aux FREAD = \
"push ds" \
"push es" \
"pop ds" \
"mov ah, 0x3F"    /* read cx bytes from file handle bx to DS:DX */ \
"int 0x21" \
"jnc ERR" \
"xor ax, ax"      /* return 0 on error */ \
"ERR:" \
"pop ds" \
parm [bx] [es dx] [cx] \
value [ax]


static void FSEEK(unsigned short handle, unsigned short bytes);

#pragma aux FSEEK = \
"mov ax, 0x4201"  /* move file pointer from cur pos + CX:DX */ \
"xor cx, cx" \
"int 0x21" \
parm [bx] [dx] \
modify [ax cx dx]

#endif


int svarlang_load(const char *fname, const char *lang) {
  unsigned short langid;
  unsigned short buff16[2];
  FHANDLE fd;
  signed char exitcode = 0;
  struct {
    unsigned long sig;
    unsigned short string_count;
  } hdr;

  langid = *((unsigned short *)lang);
  langid &= 0xDFDF; /* make sure lang is upcase */

  fd = FOPEN(fname);
  if (!fd) return(-1);

  /* read hdr, sig should be "SvL\x1a" (0x1a4c7653) */
  if ((FREAD(fd, &hdr, 6) != 6) || (hdr.sig != 0x1a4c7653L) || (hdr.string_count != svarlang_string_count)) {
    exitcode = -2;
    goto FCLOSE_AND_EXIT;
  }

  for (;;) {
    /* read next lang id and string table size in file */
    if (FREAD(fd, buff16, 4) != 4) {
      exitcode = -3;
      goto FCLOSE_AND_EXIT;
    }

    /* is it the lang I am looking for? */
    if (buff16[0] == langid) break;

    /* skip to next lang (in two steps to avoid a potential uint16 overflow) */
    FSEEK(fd, svarlang_string_count * 4);
    FSEEK(fd, buff16[1]);
  }

  /* load dictionary & strings, but only if I have enough memory space */
  if ((buff16[1] >= svarlang_memsz)
   || (FREAD(fd, svarlang_dict, svarlang_string_count * 4) != svarlang_string_count * 4)
   || (FREAD(fd, svarlang_mem, buff16[1]) != buff16[1])) {
    exitcode = -4;
  }

  FCLOSE_AND_EXIT:

  FCLOSE(fd);
  return(exitcode);
}
